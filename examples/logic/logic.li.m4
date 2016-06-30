define(`IFEQ', `
$2 := $2 - $1;
if $2 then
	$4
else
	$3
end
')

define(`AND', `
IFEQ(1, $1, IFEQ(1, $2, rs := 1, rs := 0), rs := 0)
')

define(`OR', `
IFEQ(1, $1, rs := 1, IFEQ(1, $2, rs := 1, rs := 0))
')

define(`NOT',
IFEQ(1, $1, rs := 0, rs := 1)
)

# Valid boolean values are 0 and 1, however, user input is not verified.
# 0 means false in this case and 1 means true. E.g. AND(1, 1) sets rs to 1.

let af := 0; # First boolean value.
let as := 0; # Second boolean value only read for binary operations.

# Boolean operator:
# 0 -> NOT (unarry operator)
# 1 -> AND (binary operator)
# 2 -> OR  (binary operator)
let op := 0;

# Result as a boolean value.
let rs := 0;

? af;
? op;

let opcopy := op;
IFEQ(0, op,
	NOT(af),
	rs := 0 - 1);
op := opcopy;

if rs + 1 then
	! rs
else
	? as;

	IFEQ(1, op,
		AND(af, as),
		rs := 0 - 1);
	op := opcopy;

	if rs + 1 then
		! rs
	else
		IFEQ(2, op,
			OR(af, as),
			rs := 0 - 1);
		op := opcopy;

		! rs
	end
end
