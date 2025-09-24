#include <exception>
#include <list>
#include "util/macro.hh"
#include "pump/flat.hh"
#include "pump/repeat.hh"
#include "pump/sequential.hh"
#include "pump/reduce.hh"
#include "pump/visit.hh"
#include "pump/just.hh"
#include "pump/when_all.hh"
#include "pump/submit.hh"
#include "sider/coro/coro.hh"
#include "sider/cuda/common/wait_stream_done.hh"

#include "cuda_runtime.h"


using namespace pump::coro;
using namespace sider::pump;
using namespace sider::meta;
using namespace sider::cuda;

#define N 10000000
template <typename T>
__global__
void vector_add(T *out, T *a, T *b, int n) {

    for(int i = 0; i < n; i++){
        out[i] = a[i] * b[i];
    }
    printf("1111  %d,%d\n", (int) out[0], (int) out[1]);
}

struct
compute_unit {
    float *g_a, *g_b, *g_o;
    cudaStream_t stream;
};
int
main(int argc, char **argv) {


    std::list<compute_unit*> units;
    just()
        >> for_each(units)
        >> then([](compute_unit* u){

        })
        >> reduce();

    float *c_a, *c_b, *c_o;
    float *g_a, *g_b, *g_o;

    cudaStream_t stream{0};
    cudaStreamCreate(&stream);

    just()
        >> then([&]() {
            c_a   = (float*)malloc(sizeof(float) * N);
            cudaMalloc((void **) &g_a, sizeof(float) * N);

            cudaPointerAttributes attr;
            auto e = cudaPointerGetAttributes(&attr, g_a);

            memcpy(attr.devicePointer, c_a, sizeof(float) * N);

            c_b   = (float*)malloc(sizeof(float) * N);
            cudaMalloc((void **) &g_b, sizeof(float) * N);
            c_o   = (float*)malloc(sizeof(float) * N);
            cudaMalloc((void **) &g_o, sizeof(float) * N);

            for(int i = 0; i < N; i++){
                c_a[i] = 1.0f; c_b[i] = 2.0f;
            }

            cudaMemcpyAsync(g_a, c_a, sizeof(float) * N, cudaMemcpyHostToDevice, stream);
            cudaMemcpyAsync(g_b, c_b, sizeof(float) * N, cudaMemcpyHostToDevice, stream);
            vector_add<<<1, 1, 0, stream>>>(g_o, g_a, g_b, N);
            cudaMemcpyAsync(c_o, g_o, sizeof(float) * N, cudaMemcpyDeviceToHost, stream);
        })
        >> common::wait_stream_done(stream)
        >> then([](){
            std::cout << "done" << std::endl;
        })
        >> submit(make_root_context());

    sleep(100);
    return 0;
}