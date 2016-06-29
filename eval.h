typedef enum {
	EVAL_OK,
	ERR_SCANF,
	ERR_FLUSH,
	ERR_PRINTF,
	ERR_UNDEFVAR,
	ERR_UNINITVAR,
	ERR_PRECISION,
	ERR_DIVBYZERO,
	ERR_VARDEFINED,
} evalerr;

evalerr eval (statement **s);
