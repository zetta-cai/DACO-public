# Clear for segcache
sudo rm -rf src/cache/segcache/build # Clear cmake caches and compilations

# Clear for glcache
sudo rm -rf src/cache/glcache/micro-implementation/_build # Clear cmake caches and compilations
sudo rm -rf src/cache/glcache/micro-implementation/build # Clear cmake installations

# Clear for lrb
sudo rm -rf lib/lrb/lib/libbf/build/* # Clear cmake caches and compilations
sudo rm -rf lib/lrb/lib/LightGBM/build/* # Clear cmake caches and compilations
sudo rm -rf lib/lrb/lib/mongo-c-driver/cmake-build/* # Clear cmake caches and compilations
sudo rm -rf lib/lrb/lib/mongo-cxx-driver/build/* # Clear cmake caches and compilations
sudo rm -rf lib/lrb/build/* # Clear cmake caches and compilations
sudo rm -rf lib/lrb/install # Clear cmake installations