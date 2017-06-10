# create Linux package
( cd ~/.local/lib/python3.4/*; tar cf -  ) > python3.4-personal-packages.tar
( cd /usr/local/lib/python3.4/*;  tar -cf - python3.4 )  > python3.4-local-packages.tar

