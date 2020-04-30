#!/bin/sh -xe

die()
{
	echo $@
	exit 1
}

REGEXFILE="${1:?"regex file must be provided as 1st arg"}"
test -f "$REGEXFILE" || die "${REGEXFILE} is not a regular file"

TARGETFILE="${2:?"target file must be provided as 2nd arg"}"
test -f "$TARGETFILE" || die "${TARGETFILE} is not a regular file"

git checkout "${TARGETFILE}"
while read -r REGEX
do
	sed -i "${REGEX}" "${TARGETFILE}"
done < "${REGEXFILE}"
sed -i '1s/^/#include <vector>\n/' "${TARGETFILE}"
