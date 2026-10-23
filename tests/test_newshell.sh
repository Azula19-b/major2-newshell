#!/bin/sh
set -eu

PROGRAM_INPUT=${1:-./newshell}
case "$PROGRAM_INPUT" in
    /*) PROGRAM=$PROGRAM_INPUT ;;
    *) PROGRAM="$(pwd)/${PROGRAM_INPUT#./}" ;;
esac

TEST_DIR=$(mktemp -d "${TMPDIR:-/tmp}/newshell-tests.XXXXXX")
trap 'rm -rf "$TEST_DIR"' EXIT HUP INT TERM
mkdir "$TEST_DIR/subdir"
TEST_DIR=$(cd "$TEST_DIR" && pwd -P)

PASSED=0

pass() {
    PASSED=$((PASSED + 1))
    printf '[PASS] %s\n' "$1"
}

assert_contains() {
    name=$1
    output=$2
    expected=$3
    if printf '%s\n' "$output" | grep -F -- "$expected" >/dev/null; then
        pass "$name"
    else
        printf '[FAIL] %s: expected "%s"\n' "$name" "$expected" >&2
        exit 1
    fi
}

cat >"$TEST_DIR/features.batch" <<'EOF'
echo basic-ok
echo "quoted value"
echo first; echo second
echo "quoted;semicolon"
echo escaped\;semicolon
pwd
cd subdir
pwd
cd ..
printf 'alpha\nbeta\nalpha\n' > input.txt
cat < input.txt
cat input.txt | grep alpha | wc -l
echo first > output.txt
echo second >> output.txt
cat output.txt
alias greet echo hello
greet world
alias greet
unalias greet
path
myhistory
EOF

output=$(cd "$TEST_DIR" && "$PROGRAM" features.batch)
assert_contains "external command" "$output" "basic-ok"
assert_contains "quoted argument" "$output" "quoted value"
assert_contains "semicolon command list" "$output" "first"
assert_contains "quoted semicolon" "$output" "quoted;semicolon"
assert_contains "escaped semicolon" "$output" "escaped;semicolon"
assert_contains "cd changes shell directory" "$output" "$TEST_DIR/subdir"
assert_contains "input redirection" "$output" "beta"
assert_contains "three-stage pipeline" "$output" "2"
assert_contains "output append redirection" "$output" "second"
assert_contains "alias expansion" "$output" "hello world"
assert_contains "alias listing" "$output" "alias greet='echo hello'"
assert_contains "path built-in" "$output" "/usr"
assert_contains "history listing" "$output" "myhistory"

if [ "$(cat "$TEST_DIR/output.txt")" = "first
second" ]; then
    pass "redirection file contents"
else
    printf '[FAIL] redirection file contents\n' >&2
    exit 1
fi

cat >"$TEST_DIR/history.batch" <<'EOF'
echo replay-me
myhistory 1
EOF
history_output=$(cd "$TEST_DIR" && "$PROGRAM" history.batch)
if [ "$(printf '%s\n' "$history_output" | grep -c '^replay-me$')" -eq 2 ]; then
    pass "history replay"
else
    printf '[FAIL] history replay\n' >&2
    exit 1
fi

interactive_output=$(printf 'echo interactive-ok\nexit 0\n' | "$PROGRAM")
assert_contains "interactive mode" "$interactive_output" "interactive-ok"

if "$PROGRAM" "$TEST_DIR/missing.batch" >/dev/null 2>&1; then
    printf '[FAIL] missing batch file should fail\n' >&2
    exit 1
else
    pass "missing batch-file error"
fi

printf 'exit 7\n' >"$TEST_DIR/exit.batch"
set +e
(cd "$TEST_DIR" && "$PROGRAM" exit.batch >/dev/null 2>&1)
exit_status=$?
set -e
if [ "$exit_status" -eq 7 ]; then
    pass "exit status propagation"
else
    printf '[FAIL] exit status propagation: got %s\n' "$exit_status" >&2
    exit 1
fi

printf 'echo "unterminated\n' >"$TEST_DIR/invalid.batch"
if (cd "$TEST_DIR" && "$PROGRAM" invalid.batch >/dev/null 2>&1); then
    printf '[FAIL] unmatched quotes should fail\n' >&2
    exit 1
else
    pass "parse-error status"
fi

printf '\n%d tests passed, 0 failed\n' "$PASSED"
