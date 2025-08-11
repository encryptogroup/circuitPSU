cd securejoin
mkdir -p ../libs/securejoin
python3 build.py --install=../libs/securejoin
cp out/build/linux/secure-join/libsecureJoin.a ../libs/securejoin/lib/libsecureJoin.a
cd secure-join
mkdir ../../libs/securejoin/include/secure-join
cp -r * ../../libs/securejoin/include/secure-join
rm ../../libs/securejoin/include/secure-join/config.h
echo "#pragma once" > ../../libs/securejoin/include/secure-join/config.h
