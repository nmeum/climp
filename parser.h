typedef enum {
	OP_PLUS,
	OP_MINUS,
	OP_MULTI,
	OP_DIVIDE,
} binop;

typedef enum {
	EXP_LIT,
	EXP_VAR,
	EXP_BIN,
} expr_t;

typedef struct parser parser;

struct parser {
	scanner *scr;
	token *peektok;
};

typedef struct expr expr;

struct expr {
	expr_t type;
	union {
		struct {
			int num;
		} literal;
		struct {
			char *var;
		} variable;
		struct {
			binop op;
			expr *expr1, *expr2;
		} operation;
	} d;
};

typedef enum {
	STMT_ERROR,
	STMT_DEFINE,
	STMT_ASSIGN,
	STMT_READ,
	STMT_WRITE,
	STMT_COND,
	STMT_LOOP,
} stmt_t;

typedef struct statement statement;

struct statement {
	stmt_t type;
	union {
		struct {
			int line;
			char *msg;
		} error;
		struct {
			expr *exp;
			char *var;
		} define;
		struct {
			expr *exp;
			char *var;
		} assign;
		struct {
			char *var;
		} read;
		struct {
			expr *exp;
		} write;
		struct {
			char *cond;
			statement *cmd1, *cmd2;
		} cond;
		struct {
			char *cond;
			statement *body;
		} loop;
	} d;
};
