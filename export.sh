rm -rf /tmp/export
mkdir /tmp/export
svn export fceu /tmp/export/fceu
cd /tmp/export
tar -cjf ../export.src.tar.bz2 *
