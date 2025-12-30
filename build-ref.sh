#!/usr/bin/env -S bash -Ee

if [ -f $1 ]; then
    echo "$1" already exists 1>&2
    echo $1
    exit 0
fi
#elif make --dry-run $1; then
#    make --dry-run $1
#    echo $1
#    exit 0
#fi

TARGET_DIR=release

if [ ${TARGET_DIR} == "debug" ]; then
    PROFILE=dev
else
    PROFILE=${TARGET_DIR}
fi

echo Parsing reference "$1" 1>&2

HASH=$(git rev-parse --verify "$1")
WORKTREE_PATH=/tmp/thera-${HASH}

ORIGINA_PWD=$(pwd)

if [ ! -d ${WORKTREE_PATH} ]; then
    git clone . --revision ${HASH} ${WORKTREE_PATH} 1>&2
fi

cd ${WORKTREE_PATH}
cargo build --profile ${PROFILE} --bin thera

OUTPUT_BINARY=$(pwd)/target/${TARGET_DIR}/thera

echo ${OUTPUT_BINARY}
