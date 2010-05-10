#!/bin/sh
# compile a test file into a shell script.
# A test file is a text file with several string variables.
# The shell script simply replaces the markers within boilerplate code.
# The format is as follows: 
#
# _PURPOSE_
# some text explaining the test in case it fails
# _INPUT_ 
# some lines representing an input file on stdin (verbatim)
# _COMMAND_
# a command (possibly a pipeline, verbatim)
# _EXITCODE_
# 0 (a number, if nonzero, then OUTPUT won't be tested)
# _OUTPUT_
# some lines representing the required output (verbatim)
# _END_

# WATCH QUOTING RULES: 
# 1) COMMAND must be directly executable by sh

IFS=' ' # important for read command

# skip initial comments
while read junk; do
    if test "$junk" = "_PURPOSE_" ; then
	break
    fi
done

# read purpose 
while read junk; do
    if test "$junk" = "_INPUT_" ; then
	break
    fi
    PURPOSE="$PURPOSE$junk"
done


cat <<EOF1
#!/bin/sh
PATH=\$TESTBIN:/bin:/usr/bin
TMP_PATH="\`pwd\`/\`basename \$0 .sh\`_\`date +"%Y%m%dT%H%M%S"\`"

mkdir "\$TMP_PATH"

cat <<EOF_INPUT > "\$TMP_PATH/input"
EOF1

# read input 
while read -r junk; do
    if test "$junk" = "_COMMAND_" ; then
	break
    fi
    echo $junk
done

cat <<EOF2
EOF_INPUT
EOF2

# read command
while read -r junk; do
    if test "$junk" = "_EXITCODE_" ; then
	break
    fi
    COMMAND="$COMMAND $junk"
done

# read exit code
while read -r junk; do
    if test "$junk" = "_OUTPUT_" ; then
	break
    fi
    EXITCODE="$junk"
done

cat <<EOF3 
cat -E <<EOF_OUTPUT > "\$TMP_PATH/output.expected"
EOF3

# read output 
while read -r junk; do
    if test "$junk" = "_END_" ; then
	break
    fi
    echo "$junk"
done

cat <<EOF4
EOF_OUTPUT

cat "\$TMP_PATH/input" | $COMMAND 2>> "\$TMP_PATH/error" > "\$TMP_PATH/output"

RESULT="\$?"

if test "\$RESULT" != "$EXITCODE" ; then
  echo Purpose: "$PURPOSE"
  echo -n "Error: Unexpected exit code. "
  cat "\$TMP_PATH/error"

  rm -rf "\$TMP_PATH"
  exit 1
fi

if test "$EXITCODE" != "0" ; then
  rm -rf "\$TMP_PATH"
  exit 0
fi

cat -E "\$TMP_PATH/output" > "\$TMP_PATH/outputE"

diff "\$TMP_PATH/output.expected" "\$TMP_PATH/outputE" > "\$TMP_PATH/output.diff"

RESULT="\$?"

if test "\$RESULT" != "0" ; then
  echo Purpose: "$PURPOSE"
  echo "Differences: Expected(<) vs Actual(>)"
  cat "\$TMP_PATH/output.diff" 
  echo -n "ErrorMessage: "; cat "\$TMP_PATH/error"
fi

rm -rf "\$TMP_PATH"
exit \$RESULT
EOF4
