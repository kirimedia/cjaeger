#!/bin/sh
set -xe

SPECFILE=$1
NAME=`basename -s .spec $SPECFILE`
VERSION=`cat CMakeLists.txt | grep -oP "$NAME VERSION ([0-9.]*)" | cut -d ' ' -f 3`

BRANCH="${GIT_BRANCH:-$(git name-rev --name-only HEAD | sed -r 's/([^~\^]+)((~[[:digit:]]+)|(\^[[:digit:]]+))*/\1/g')}"
BRANCH_FOR_RPM="$(echo $BRANCH | rev | cut -d/ -f1 | rev | sed 's/-/_/g')"
LAST_COMMIT_DATETIME="$(git log -n 1 --format='%ci' | awk '{ print $1, $2 }' | sed 's/[ :]//g;s/-//g')"

err() {
  exitval="$1"
  shift
  echo "$@" > /dev/stderr
  exit $exitval
}

echo "Building \"$1\""
if [ ! -f "$1" ]; then
  err 1 "Spec \"$1\" not found"
fi

shift

CURRENT_DATETIME=`date +'%Y%m%d%H%M%S'`

if [ ! -f "$HOME/.rpmmacros" ]; then
   echo "%_topdir $HOME/rpm/" > $HOME/.rpmmacros
   echo "%_tmppath $HOME/rpm/tmp" >> $HOME/.rpmmacros
   echo "%packager ${PACKAGER}" >> $HOME/.rpmmacros
fi

if [ ! -d "$HOME/rpm" ]; then
  echo "Creating directories need by rpmbuild"
  mkdir -p ~/rpm/{BUILD,RPMS,SOURCES,SRPMS,SPECS,tmp} 2>/dev/null
  mkdir ~/rpm/RPMS/{i386,i586,i686,noarch} 2>/dev/null
fi

RPM_TOPDIR=`rpm --eval '%_topdir'`
BUILDROOT=`rpm --eval '%_tmppath'`
BUILDROOT_TMP="$BUILDROOT/tmp/"
BUILDROOT="$BUILDROOT/tmp/${PACKAGE}"

PACKAGE=${NAME}-${CURRENT_DATETIME}
[ "$PACKAGE" != "/" ] && [ -d "$PACKAGE" ] && rm -rf "$PACKAGE"

mkdir -p ${RPM_TOPDIR}/{BUILD,RPMS,SOURCES,SRPMS,SPECS}
mkdir -p ${RPM_TOPDIR}/RPMS/{i386,i586,i686,noarch}
mkdir -p $BUILDROOT

(
TEMPDIR="$HOME/rpm/tmp/"
cp -prv . "$TEMPDIR/$PACKAGE"
cd "$TEMPDIR"
tar czf ${RPM_TOPDIR}/SOURCES/${NAME}-${CURRENT_DATETIME}.tar.gz ${PACKAGE}
[ "$PACKAGE" != "/" ] && rm -rvf "$PACKAGE"
)

COMMIT_TIMESTAMP=`git log -n 1 --format='%ci' | awk '{ print $1, $2 }' | sed 's/[ :-]//g'`
COMMIT_HASH=`git rev-parse --short HEAD`

VERSION_SUFFIX="%{nil}"
if [ "${BRANCH_FOR_RPM}" != "master" ]; then
  if [ "${GITLAB_CI}" != "true" ]; then
    VERSION_SUFFIX=".${BRANCH_FOR_RPM}.${LAST_COMMIT_DATETIME}"
  fi
fi

rpmbuild -ba --clean $SPECFILE \
  --define "current_datetime ${CURRENT_DATETIME}" \
  --define "version ${VERSION}" \
  --define "version_suffix ${VERSION_SUFFIX}"
