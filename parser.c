#include "parser.h"
#include "io.h"
#include "lexgen/runtime-src/runtime.h"
#define LEXGEN_TRANSITION_TABLE_IMPLEMENTATION
#include "grammar.h"
#include "shl_log.h"
#include "shl_arena.h"

#define MASK(id) (1 << (id))

typedef struct {
  u64   id;
  Str   lexeme;
  u32   row, col;
  char *file_path;
} Token;

typedef Da(Token) Tokens;

typedef Da(Str) Strs;

typedef struct {
  Str     name;
  Strs    args;
  IrBlock body;
  bool    has_unpack;
} Macro;

typedef Da(Macro) Macros;

typedef struct {
  Tokens tokens;
  u32    index;
  Ir     ir;
  Macros macros;
} Parser;

static char *token_names[] = {
  "whitespace",
  "new line",
  "comment",
  "fun",
  "let",
  "if",
  "elif",
  "else",
  "macro",
  "while",
  "use",
  "set",
  "field",
  "int",
  "float",
  "bool",
  "identifier",
  "key",
  "macro_name",
  "`(`",
  "`)`",
  "`[`",
  "`]`",
  "`{`",
  "`}`",
  "string literal",
  "...",
};

static char escape_char(char _char) {
  switch (_char) {
  case 'n': return '\n';
  case 'r': return '\r';
  case 't': return '\t';
  case 'v': return '\v';
  default:  return _char;
  }
}

static Tokens lex(Str code, char *file_path) {
  Tokens tokens = {0};
  TransitionTable *table = get_transition_table();
  u32 row = 0, col = 0;

  while (code.len > 0) {
    Token new_token = {0};
    new_token.lexeme = table_matches(table, &code, &new_token.id);
    new_token.row = row;
    new_token.col = col;
    new_token.file_path = file_path;

    if (new_token.id == (u64) -1) {
      ERROR("Unexpected `%c` at %u:%u\n", code.ptr[0], row + 1, col + 1);
      exit(1);
    }

    if (new_token.id == TT_STR) {
      StringBuilder sb = {0};
      sb_push_char(&sb, code.ptr[0]);

      bool is_escaped = false;
      while (code.len > 0 &&
             ((code.ptr[0] != '"' &&
               code.ptr[0] != '\'') ||
              is_escaped)) {

        if (is_escaped || code.ptr[0] != '\\') {
          if (is_escaped)
            code.ptr[0] = escape_char(code.ptr[0]);
          sb_push_char(&sb, code.ptr[0]);
        }

        if (is_escaped)
          is_escaped = false;
        else if (code.ptr[0] == '\\')
          is_escaped = true;

        ++code.ptr;
        --code.len;
      }

      if (code.len == 0) {
        ERROR("String literal at %u:%u was not closed\n",
              new_token.row + 1, new_token.col + 1);
        exit(1);
      }

      ++code.ptr;
      --code.len;

      sb_push_char(&sb, code.ptr[0]);

      new_token.lexeme = sb_to_str(sb);
    }

    col += new_token.lexeme.len;
    if (new_token.id == TT_NEWLINE) {
      ++row;
      col = 0;
    } else if (new_token.id == TT_COMMENT) {
      while (code.len > 0 && code.ptr[0] != '\n') {
        ++code.ptr;
        --code.len;
      }
    } else if (new_token.id != TT_WHITESPACE) {
      DA_APPEND(tokens, new_token);
    }
  }

  return tokens;
}

static Token *parser_peek_token(Parser *parser) {
  if (parser->index >= parser->tokens.len)
    return NULL;
  return parser->tokens.items + parser->index;
}

static Token *parser_next_token(Parser *parser) {
  if (parser->index >= parser->tokens.len)
    return NULL;
  return parser->tokens.items + parser->index++;
}

static void print_id_mask(u64 id_mask) {
  u32 ids_count = 0;
  for (u32 i = 0; i < ARRAY_LEN(token_names); ++i)
    if (MASK(i) & id_mask)
      ++ids_count;

  for (u32 i = 0, j = 0; i < 64 && j < ids_count; ++i) {
    if ((1 << i) & id_mask) {
      if (j > 0) {
        if (j + 1 == ids_count)
          fputs(" or ", stderr);
        else
          fputs(", ", stderr);
      }

      fputs(token_names[i], stderr);

      ++j;
    }
  }
}

static Token *parser_expect_token(Parser *parser, u64 id_mask) {
  Token *token = parser_next_token(parser);
  if (!token) {
    PERROR("%s:%u:%u: ", "Expected ",
           token->file_path,
           token->row + 1, token->col + 1);
    print_id_mask(id_mask);
    fprintf(stderr, ", but got EOF\n");
    exit(1);
  }

  if (MASK(token->id) & id_mask)
    return token;

  PERROR("%s:%u:%u: ", "Expected ",
         token->file_path,
         token->row + 1, token->col + 1);
  print_id_mask(id_mask);
  fprintf(stderr, ", but got `"STR_FMT"`\n",
          STR_ARG(token->lexeme));
  exit(1);
}

static IrBlock parser_parse_block(Parser *parser, u64 end_id_mask);

static Ir parser_parse(Parser *parser, Str code, char *file_path) {
  parser->tokens = lex(code, file_path);
  parser->ir = parser_parse_block(parser, 0);

  return parser->ir;
}

static IrExprFuncDef parser_parse_func_def(Parser *parser) {
  IrExprFuncDef func_def = {0};

  parser_expect_token(parser, MASK(TT_FUN));

  if (parser_peek_token(parser)->id == TT_IDENT) {
    Token *name_token = parser_next_token(parser);
    func_def.name = name_token->lexeme;
  }

  parser_expect_token(parser, MASK(TT_OBRACKET));

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_CBRACKET) {
    Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT));
    DA_APPEND(func_def.args, arg_token->lexeme);

    next_token = parser_peek_token(parser);
  }

  parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_CBRACKET));
  func_def.body = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  return func_def;
}

static IrExprFuncCall parser_parse_func_call(Parser *parser) {
  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  IrBlock args = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  return (IrExprFuncCall) {
    name_token->lexeme,
    args,
  };
}

static void parser_parse_macro_def(Parser *parser) {
  Macro macro = {0};

  parser_expect_token(parser, MASK(TT_MACRO));

  Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));
  macro.name = name_token->lexeme;

  parser_expect_token(parser, MASK(TT_OBRACKET));

  Token *next_token = parser_peek_token(parser);
  while (next_token && next_token->id != TT_CBRACKET) {
    Token *arg_token = parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_UNPACK));
    if (arg_token->id == TT_UNPACK) {
      macro.has_unpack = true;

      arg_token = parser_expect_token(parser, MASK(TT_IDENT));
    }

    DA_APPEND(macro.args, arg_token->lexeme);

    next_token = parser_peek_token(parser);
  }

  parser_expect_token(parser, MASK(TT_IDENT) | MASK(TT_CBRACKET));
  macro.body = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  DA_APPEND(parser->macros, macro);
}

static void macro_body_expand_block(IrBlock *block, IrBlock *args,
                                    Strs *arg_names, bool unpack);

static void macro_body_expand(IrExpr **expr, IrBlock *args, Strs *arg_names,
                              IrBlock *parent_block, bool unpack) {
  IrExpr *new_expr = aalloc(sizeof(IrExpr));
  *new_expr = **expr;
  *expr = new_expr;

  switch (new_expr->kind) {
  case IrExprKindBlock: {
    macro_body_expand_block(&new_expr->as.block, args, arg_names, unpack);
  } break;

  case IrExprKindFuncDef: {
    macro_body_expand_block(&new_expr->as.func_def.body, args, arg_names, unpack);
  } break;

  case IrExprKindFuncCall: {
    macro_body_expand_block(&new_expr->as.func_call.args, args, arg_names, unpack);
  } break;

  case IrExprKindVarDef: {
    macro_body_expand(&new_expr->as.var_def.expr, args, arg_names, NULL, unpack);
  } break;

  case IrExprKindIf: {
    macro_body_expand_block(&new_expr->as._if.if_body, args, arg_names, unpack);

    for (u32 i = 0; i < new_expr->as._if.elifs.len; ++i)
      macro_body_expand_block(&new_expr->as._if.elifs.items[i].body,
                              args, arg_names, unpack);

    if (new_expr->as._if.has_else)
      macro_body_expand_block(&new_expr->as._if.else_body, args, arg_names, unpack);
  } break;

  case IrExprKindWhile: {
    macro_body_expand_block(&new_expr->as._while.body, args, arg_names, unpack);
  } break;

  case IrExprKindSet: {
    macro_body_expand(&new_expr->as.set.src, args, arg_names, NULL, unpack);
  } break;

  case IrExprKindField: {
    if (new_expr->as.field.is_set)
      macro_body_expand(&new_expr->as.field.expr, args, arg_names, NULL, unpack);
  } break;

  case IrExprKindList: {
    macro_body_expand_block(&new_expr->as.list.content, args, arg_names, unpack);
  } break;

  case IrExprKindIdent: {
    u32 index = (u32) -1;
    for (u32 i = 0; i < arg_names->len; ++i) {
      if (str_eq(new_expr->as.ident.ident, arg_names->items[i])) {
        index = i;
        break;
      }
    }

    if (index != (u32) -1) {
      if (index + 1 == arg_names->len && unpack) {
        if (!parent_block) {
          ERROR("Macro arguments can be unpacked only inside of a block\n");
          exit(1);
        }

        parent_block->len -= 1;

        IrBlock *block = &args->items[args->len - 1]->as.block;
        for (u32 i = 0; i < block->len; ++i) {
          DA_APPEND(*parent_block, block->items[i]);
          macro_body_expand(parent_block->items + parent_block->len - 1,
                            args, arg_names, NULL, unpack);
        }
      } else {
        *new_expr = *args->items[index];
      }
    }
  } break;

  case IrExprKindString: break;
  case IrExprKindInt: break;
  case IrExprKindFloat: break;
  case IrExprKindBool:   break;

  case IrExprKindLambda: {
    macro_body_expand_block(&new_expr->as.lambda.body, args, arg_names, unpack);
  } break;

  case IrExprKindRecord: {
    for (u32 i = 0; new_expr->as.record.len; ++i)
      macro_body_expand(&new_expr->as.record.items[i].expr, args, arg_names, NULL, unpack);
  } break;
  }
}

static void macro_body_expand_block(IrBlock *block, IrBlock *args,
                                    Strs *arg_names, bool unpack) {
  for (u32 i = 0; i < block->len; ++i)
    macro_body_expand(block->items + i, args, arg_names, block, unpack);
}

static IrBlock parser_parse_macro_expand(Parser *parser) {
  Token *name_token = parser_expect_token(parser, MASK(TT_MACRO_NAME));
  Str name = name_token->lexeme;
  ++name.ptr;
  --name.len;

  IrBlock args = parser_parse_block(parser, MASK(TT_CPAREN));
  parser_expect_token(parser, MASK(TT_CPAREN));

  Macro *macro = NULL;

  for (u32 i = 0; i < parser->macros.len; ++i) {
    Macro *temp_macro = parser->macros.items + i;

    if (str_eq(temp_macro->name, name) &&
        (temp_macro->args.len == args.len ||
         (temp_macro->args.len <= args.len &&
          temp_macro->has_unpack))) {
      macro = temp_macro;
      break;
    }
  }

  if (!macro) {
    ERROR("Macro "STR_FMT" with %u arguments was not defined before usage\n",
          STR_ARG(name), args.len);
    exit(1);
  }

  if (macro->has_unpack) {
    IrExpr *unpack_body = aalloc(sizeof(IrExpr));
    unpack_body->kind = IrExprKindBlock;
    for (u32 i = 0; i < args.len - macro->args.len + 1; ++i)
      DA_APPEND(unpack_body->as.block, args.items[i]);

    args.items[macro->args.len - 1] = unpack_body;
    args.len = macro->args.len;
  }

  IrBlock body = macro->body;
  macro_body_expand_block(&body, &args, &macro->args, macro->has_unpack);
  return body;
}

static IrExpr *parser_parse_expr(Parser *parser);

static IrExprRecord parser_parse_record(Parser *parser) {
  IrExprRecord record = {0};

  Token *token;
  while ((token = parser_peek_token(parser))->id != TT_CCURLY) {
    Token *key_token = parser_expect_token(parser, MASK(TT_KEY));
    Str key = key_token->lexeme;
    --key.len;

    IrExpr *expr = parser_parse_expr(parser);

    IrField field = { key, expr };
    DA_APPEND(record, field);
  }

  parser_expect_token(parser, MASK(TT_CCURLY));

  return record;
}

static char *str_to_cstr(Str str) {
  char *result = malloc((str.len + 1) * sizeof(char));
  memcpy(result, str.ptr, str.len * sizeof(char));
  result[str.len] = 0;

  return result;
}

static Str get_file_dir(Str path) {
  for (u32 i = path.len; i > 0; --i)
    if (path.ptr[i - 1] == '/')
      return (Str) { path.ptr, i };

  return (Str) {0};
}

static IrExpr *parser_parse_expr(Parser *parser) {
  IrExpr *expr = aalloc(sizeof(IrExpr));
  *expr = (IrExpr) {0};

  Token *token = parser_expect_token(parser, MASK(TT_OPAREN) | MASK(TT_OBRACKET) |
                                             MASK(TT_STR) | MASK(TT_IDENT) |
                                             MASK(TT_INT) | MASK(TT_FLOAT) |
                                             MASK(TT_BOOL) | MASK(TT_OCURLY));

  if (token->id == TT_STR) {
    expr->kind = IrExprKindString;
    expr->as.string.lit = STR(token->lexeme.ptr + 1,
                              token->lexeme.len - 2);

    return expr;
  }

  if (token->id == TT_IDENT) {
    expr->kind = IrExprKindIdent;
    expr->as.ident.ident = token->lexeme;

    return expr;
  }

  if (token->id == TT_INT) {
    expr->kind = IrExprKindInt;
    expr->as._int._int = str_to_i64(token->lexeme);

    return expr;
  }

  if (token->id == TT_FLOAT) {
    expr->kind = IrExprKindFloat;
    expr->as._float._float = str_to_f64(token->lexeme);

    return expr;
  }

  if (token->id == TT_BOOL) {
    expr->kind = IrExprKindBool;
    expr->as._bool._bool = str_eq(token->lexeme, STR_LIT("true"));

    return expr;
  }

  if (token->id == TT_OBRACKET) {
    expr->kind = IrExprKindList;
    expr->as.list.content = parser_parse_block(parser, MASK(TT_CBRACKET));
    parser_expect_token(parser, MASK(TT_CBRACKET));

    return expr;
  }

  if (token->id == TT_OCURLY) {
    expr->kind = IrExprKindRecord;
    expr->as.record = parser_parse_record(parser);

    return expr;
  }

  token = parser_peek_token(parser);
  switch (token->id) {
  case TT_FUN: {
    IrExprFuncDef func_def = parser_parse_func_def(parser);

    if (func_def.name.len > 0) {
      expr->kind = IrExprKindFuncDef;
      expr->as.func_def = func_def;
    } else {
      expr->kind = IrExprKindLambda;
      expr->as.lambda = (IrExprLambda) { func_def.args, func_def.body };
    }
  } break;

  case TT_LET: {
    parser_next_token(parser);
    Token *name_token = parser_expect_token(parser, MASK(TT_IDENT));

    expr->kind = IrExprKindVarDef;
    expr->as.var_def.name = name_token->lexeme;
    expr->as.var_def.expr = parser_parse_expr(parser);

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_IF: {
    parser_next_token(parser);

    expr->kind = IrExprKindIf;
    expr->as._if.cond = parser_parse_expr(parser);

    expr->as._if.if_body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                                      MASK(TT_ELIF) |
                                                      MASK(TT_ELSE));

    Token *next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                                    MASK(TT_ELIF) |
                                                    MASK(TT_ELSE));

    while (next_token->id == TT_ELIF) {
      IrElif elif;
      elif.cond = parser_parse_expr(parser);
      elif.body = parser_parse_block(parser, MASK(TT_CPAREN) |
                                             MASK(TT_ELIF) |
                                             MASK(TT_ELSE));
      DA_APPEND(expr->as._if.elifs, elif);

      next_token = parser_expect_token(parser, MASK(TT_CPAREN) |
                                               MASK(TT_ELIF) |
                                               MASK(TT_ELSE));
    }

    expr->as._if.has_else = next_token->id == TT_ELSE;
    if (expr->as._if.has_else) {
      expr->as._if.else_body = parser_parse_block(parser, MASK(TT_CPAREN));
      parser_expect_token(parser, MASK(TT_CPAREN));
    }
  } break;

  case TT_IDENT: {
    expr->kind = IrExprKindFuncCall;
    expr->as.func_call = parser_parse_func_call(parser);
  } break;

  case TT_MACRO_NAME: {
    expr->kind = IrExprKindBlock;
    expr->as.block = parser_parse_macro_expand(parser);
  } break;

  case TT_BOOL: {
    expr->kind = IrExprKindBool;
    expr->as._bool._bool = str_eq(token->lexeme, STR_LIT("true"));
  } break;

  case TT_MACRO: {
    parser_parse_macro_def(parser);

    return NULL;
  } break;

  case TT_WHILE: {
    parser_next_token(parser);

    expr->kind = IrExprKindWhile;
    expr->as._while.cond = parser_parse_expr(parser);
    expr->as._while.body = parser_parse_block(parser, MASK(TT_CPAREN));

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_SET: {
    parser_next_token(parser);

    expr->kind = IrExprKindSet;
    expr->as.set.dest = parser_expect_token(parser, MASK(TT_IDENT))->lexeme;
    expr->as.set.src = parser_parse_expr(parser);

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  case TT_USE: {
    parser_next_token(parser);

    Str current_file_path = {
      token->file_path,
      strlen(token->file_path),
    };

    Str new_file_path = parser_expect_token(parser, MASK(TT_STR))->lexeme;
    new_file_path.ptr += 1;
    new_file_path.len -= 2;

    StringBuilder path_sb = {0};
    sb_push_str(&path_sb, get_file_dir(current_file_path));
    sb_push_str(&path_sb, new_file_path);
    char *path = str_to_cstr(sb_to_str(path_sb));

    Str code = read_file(path);
    if (code.len == (u32) -1) {
      ERROR("File %s was not found\n", path);
      exit(1);
    }

    expr->kind = IrExprKindBlock;
    expr->as.block = parser_parse(parser, code, path);

    parser_expect_token(parser, MASK(TT_CPAREN));

    free(path_sb.buffer);
  } break;

  case TT_FIELD: {
    parser_next_token(parser);

    expr->kind = IrExprKindField;
    expr->as.field.record = parser_parse_expr(parser);
    expr->as.field.field = parser_expect_token(parser, MASK(TT_IDENT))->lexeme;

    if (parser_peek_token(parser)->id != TT_CPAREN) {
      expr->as.field.is_set = true;
      expr->as.field.expr = parser_parse_expr(parser);
    }

    parser_expect_token(parser, MASK(TT_CPAREN));
  } break;

  default: {
    parser_expect_token(parser, MASK(TT_FUN) | MASK(TT_LET) |
                                MASK(TT_IF) | MASK(TT_IDENT) |
                                MASK(TT_MACRO_NAME) | MASK(TT_BOOL) |
                                MASK(TT_MACRO) | MASK(TT_WHILE) |
                                MASK(TT_SET) | MASK(TT_USE) |
                                MASK(TT_FIELD));
  } break;
  }

  return expr;
}

static IrBlock parser_parse_block(Parser *parser, u64 end_id_mask) {
  IrBlock block = {0};

  Token *token = parser_peek_token(parser);
  while (token && !(MASK(token->id) & end_id_mask)) {
    IrExpr *expr = parser_parse_expr(parser);
    if (expr)
      DA_APPEND(block, expr);

    token = parser_peek_token(parser);
  }

  return block;
}

Ir parse(Str code, char *file_path) {
  Parser parser = {0};
  return parser_parse(&parser, code, file_path);
}
