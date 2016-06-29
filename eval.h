typedef enum {
	EVAL_OK,
	ERR_PRINTF,
	ERR_UNDEFVAR,
	ERR_DIVBYZERO,
	ERR_VARDEFINED,
} evalerr;

evalerr eval (statement **s);
