
project(cuda.example CUDA)
add_executable(cuda.example cuda.cu)
set_target_properties(cuda.example
        PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        LINKER_LANGUAGE CXX
)
