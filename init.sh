ROOT=$PWD
git submodule init
git submodule update
rm src/package/pikascript/pikascript-api -rf
mkdir src/package/pikascript/pikascript-api
cd src/package/pikascript/pikascript-core
git checkout master
cd $ROOT
rm build -rf
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cd $ROOT
sh make.sh
