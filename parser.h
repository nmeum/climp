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
	CMD_ERROR,
	CMD_DEFINE,
	CMD_ASSIGN,
	CMD_READ,
	CMD_WRITE,
	CMD_COND,
	CMD_LOOP,
} cmd_t;

typedef struct command command;

struct command {
	cmd_t type;
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
			command *cmd1, *cmd2;
		} cond;
		struct {
			char *cond;
			command *body;
		} loop;
	} d;
};
