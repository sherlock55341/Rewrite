#include "aig.hpp"

RW_NAMESPACE_START

void AIGManager::resetManager() {
    designName_ = "";
    designPath_ = "";
    comments_ = "";

    nObjs = 0;
    nPIs = 0;
    nPOs = 0;
    nLatches = 0;
    nNodes = 0;
    nLevels = 0;

    if (pFanin0) {
        free(pFanin0);
    }
    pFanin0 = nullptr;
    if (pFanin1) {
        free(pFanin1);
    }
    pFanin1 = nullptr;
    if (pNumFanouts) {
        free(pNumFanouts);
    }
    pNumFanouts = nullptr;
    if (pOuts) {
        free(pOuts);
    }
    pOuts = nullptr;

    if(d_pFanin0){
        cudaFree(d_pFanin0);
    }
    d_pFanin0 = nullptr;
    if(d_pFanin1){
        cudaFree(d_pFanin1);
    }
    d_pFanin1 = nullptr;
    if(d_pNumFanouts){
        cudaFree(d_pNumFanouts);
    }
    d_pNumFanouts = nullptr;
    if(d_pOuts){
        cudaFree(d_pOuts);
    }
    d_pOuts = nullptr;

    aigCreated = false;
    
    aigOnDevice = false;  

    usingDar = false;

    aigNewest = true;
}

void AIGManager::mallocDevice(){
    if(aigOnDevice){
        printf("Error : AIG has already on device\n");
        return ;
    }
    aigOnDevice = true;
    cudaMalloc(&d_pFanin0, sizeof(int) * nObjs);
    cudaMalloc(&d_pFanin1, sizeof(int) * nObjs);
    cudaMalloc(&d_pNumFanouts, sizeof(int) * nObjs);
    cudaMalloc(&pOuts, sizeof(int) * nPOs);
}

void AIGManager::freeDevice(){
    if(!aigOnDevice){
        printf("Error : AIG has not been created on device\n");
        return ;
    }
    cudaFree(d_pFanin0);
    cudaFree(d_pFanin1);
    cudaFree(d_pNumFanouts);
    cudaFree(d_pOuts);
}

void AIGManager::copy2Device() const{
    if(!aigOnDevice){
        printf("Error : AIG has not been created on device\n");
        return ;
    }
    cudaMemcpy(d_pFanin0, pFanin0, sizeof(int) * nObjs, cudaMemcpyHostToDevice);
    cudaMemcpy(d_pFanin1, pFanin1, sizeof(int) * nObjs, cudaMemcpyHostToDevice);
    cudaMemcpy(d_pNumFanouts, pNumFanouts, sizeof(int) * nObjs, cudaMemcpyHostToDevice);
    cudaMemcpy(d_pOuts, pOuts, sizeof(int) * nPOs, cudaMemcpyHostToDevice);
}

void AIGManager::copyFromDevice() const{
    if(!aigOnDevice){
        printf("Error : AIG has not been created on device\n");
        return ;
    }
    cudaMemcpy(pFanin0, d_pFanin0, sizeof(int) * nObjs, cudaMemcpyDeviceToHost);
    cudaMemcpy(pFanin1, d_pFanin1, sizeof(int) * nObjs, cudaMemcpyDeviceToHost);
    cudaMemcpy(pNumFanouts, d_pNumFanouts, sizeof(int) * nObjs, cudaMemcpyDeviceToHost);
    cudaMemcpy(pOuts, d_pOuts, sizeof(int) * nPOs, cudaMemcpyDeviceToHost);
}

RW_NAMESPACE_END