git clone https://github.com/google/benchmark.git google_benchmark && cd google_benchmark && cmake -E make_directory "build" && cmake -E chdir "build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../ && cmake --build "build" --config Release && sudo cmake --build "build" --config Release --target install

rm google_benchmark -rf
