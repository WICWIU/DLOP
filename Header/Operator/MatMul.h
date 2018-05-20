#ifndef MATMUL_H_
#define MATMUL_H_    value

#include "..//Operator.h"
#include <cstdio>

template<typename DTYPE> class MatMul : public Operator<DTYPE>{
private:
#if __CUDNN__
    cudnnTensorDescriptor_t inputTensorDesc, outputTensorDesc, deltaDesc, inputDeltaDesc;
    cudnnConvolutionDescriptor_t convDesc;
    cudnnFilterDescriptor_t filterDesc, filterDeltaDesc;
    DTYPE *m_pDevInput, *m_pDevOutput, *m_pDevFilter, *m_pDevInputDelta, *m_pDevDelta, *m_pDevFilterDelta;
    DTYPE *m_pHostInput, *m_pHostOutput, *m_pHostFilter, *m_pHostInputDelta, *m_pHostDelta, *m_pHostFilterDelta;

    cudnnConvolutionFwdAlgo_t m_algo;
    cudnnConvolutionBwdFilterAlgo_t m_filterAlgo;
    cudnnConvolutionBwdDataAlgo_t m_dataAlgo;

    size_t m_sizeInBytes;
    size_t m_dataSizeInBytes;
    size_t m_filterSizeInBytes;

    DTYPE m_alpha;
    DTYPE m_beta;

    void *m_devWorkSpace;
    void *m_dataDevWorkSpace;
    void *m_filterDevWorkSpace;

#endif  // __CUDNN__

public:
    MatMul(Operator<DTYPE> *pWeight, Operator<DTYPE> *pInput, std::string pName) : Operator<DTYPE>(pWeight, pInput, pName) {
        #if __DEBUG__
        std::cout << "MatMul::MatMul(Operator<DTYPE> *, Operator<DTYPE> *, std::string)" << '\n';
        #endif  // __DEBUG__
        this->Alloc(pWeight, pInput);
    }

    virtual ~MatMul() {
        #if __DEBUG__
        std::cout << "Convolution2D::~Convolution2D()" << '\n';
        #endif  // __DEBUG__
        Delete();
    }

    int Alloc(Operator<DTYPE> *pWeight, Operator<DTYPE> *pInput) {
        #if __DEBUG__
        std::cout << "MatMul::Alloc(Operator<DTYPE> *, Operator<DTYPE> *)" << '\n';
        #endif  // __DEBUG__

        int timesize    = pInput->GetResult()->GetTimeSize();
        int batchsize   = pInput->GetResult()->GetBatchSize();
        int channelsize = pInput->GetResult()->GetChannelSize();
        int rowsize     = pInput->GetResult()->GetRowSize();
        int colsize     = pWeight->GetResult()->GetRowSize();

        this->SetResult(new Tensor<DTYPE>(timesize, batchsize, channelsize, rowsize, colsize));

        this->SetDelta(new Tensor<DTYPE>(timesize, batchsize, channelsize, rowsize, colsize));

        return TRUE;
    }

#if __CUDNN__
    void InitializeAttributeForGPU() {
        Operator<DTYPE> *pWeight = this->GetInput()[0];
        Operator<DTYPE> *pInput  = this->GetInput()[1];

        int timesize    = pInput->GetResult()->GetTimeSize();
        int batchsize   = pInput->GetResult()->GetBatchSize();
        int channelsize = pInput->GetResult()->GetChannelSize();
        int rowsize     = pInput->GetResult()->GetRowSize();
        int colsize     = pWeight->GetResult()->GetRowSize();

        int inputCapacity  = pInput->GetResult()->GetCapacity();
        int outputCapacity = this->GetResult()->GetCapacity();
        int filterCapacity = pWeight->GetResult()->GetCapacity();

        Shape *shapeOfWeight = pWeight->GetResult()->GetShape();
        Shape *shapeOfInput  = pInput->GetResult()->GetShape();
        Shape *shapeOfResult = this->GetResult()->GetShape();

        int rowsizeOfWeight = (*shapeOfWeight)[3];
        int colsizeOfWeight = (*shapeOfWeight)[4];

        int batchsizeOfInput = (*shapeOfInput)[1];
        int colsizeOfInput   = (*shapeOfInput)[4];

        m_sizeInBytes       = 0;
        m_dataSizeInBytes   = 0;
        m_filterSizeInBytes = 0;

        m_alpha = 1;
        m_beta  = 0;

        m_devWorkSpace       = NULL;
        m_dataDevWorkSpace   = NULL;
        m_filterDevWorkSpace = NULL;

        checkCUDNN(cudnnCreateTensorDescriptor(&inputTensorDesc));
        checkCUDNN(cudnnCreateTensorDescriptor(&outputTensorDesc));
        checkCUDNN(cudnnCreateTensorDescriptor(&deltaDesc));
        checkCUDNN(cudnnCreateTensorDescriptor(&inputDeltaDesc));
        checkCUDNN(cudnnCreateConvolutionDescriptor(&convDesc));
        checkCUDNN(cudnnCreateFilterDescriptor(&filterDesc));
        checkCUDNN(cudnnCreateFilterDescriptor(&filterDeltaDesc));

        checkCudaErrors(cudaMalloc((void **)&m_pDevInput, (inputCapacity * sizeof(DTYPE))));
        checkCudaErrors(cudaMalloc((void **)&m_pDevOutput, (outputCapacity * sizeof(DTYPE))));
        checkCudaErrors(cudaMalloc((void **)&m_pDevFilter, (filterCapacity * sizeof(DTYPE))));
        checkCudaErrors(cudaMalloc((void **)&m_pDevInputDelta, (inputCapacity * sizeof(DTYPE))));
        checkCudaErrors(cudaMalloc((void **)&m_pDevDelta, (outputCapacity * sizeof(DTYPE))));
        checkCudaErrors(cudaMalloc((void **)&m_pDevFilterDelta, (filterCapacity * sizeof(DTYPE))));

        checkCUDNN(cudnnSetTensor4dDescriptor(inputTensorDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              batchsizeOfInput, 1, 1, colsizeOfInput));

        checkCUDNN(cudnnSetFilter4dDescriptor(filterDesc, CUDNN_DATA_FLOAT, CUDNN_TENSOR_NCHW,
                                              rowsizeOfWeight, 1, 1, colsizeOfWeight));

        checkCUDNN(cudnnSetTensor4dDescriptor(outputTensorDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              batchsize, colsize, 1, 1));

        checkCUDNN(cudnnSetTensor4dDescriptor(deltaDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              batchsize, colsize, 1, 1));

        checkCUDNN(cudnnSetTensor4dDescriptor(inputDeltaDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              batchsizeOfInput, 1, 1, colsizeOfInput));

        checkCUDNN(cudnnSetFilter4dDescriptor(filterDeltaDesc, CUDNN_DATA_FLOAT, CUDNN_TENSOR_NCHW,
                                              rowsizeOfWeight, 1, 1, colsizeOfWeight));

        checkCUDNN(cudnnSetConvolution2dDescriptor(convDesc, 0, 0, 1, 1,
                                                   1, 1, CUDNN_CROSS_CORRELATION, CUDNN_DATA_FLOAT));

        checkCUDNN(cudnnGetConvolutionForwardAlgorithm(this->GetCudnnHandle(), inputTensorDesc, filterDesc, convDesc, outputTensorDesc,
                                                       CUDNN_CONVOLUTION_FWD_SPECIFY_WORKSPACE_LIMIT, 0, &m_algo));

        checkCUDNN(cudnnGetConvolutionForwardWorkspaceSize(this->GetCudnnHandle(), inputTensorDesc, filterDesc, convDesc,
                                                           outputTensorDesc, m_algo, &m_sizeInBytes));

        checkCUDNN(cudnnGetConvolutionBackwardDataAlgorithm(this->GetCudnnHandle(), filterDesc, deltaDesc, convDesc, inputDeltaDesc,
                                                            CUDNN_CONVOLUTION_BWD_DATA_SPECIFY_WORKSPACE_LIMIT, 0, &m_dataAlgo));

        checkCUDNN(cudnnGetConvolutionBackwardFilterAlgorithm(this->GetCudnnHandle(), inputTensorDesc, deltaDesc, convDesc, filterDeltaDesc,
                                                              CUDNN_CONVOLUTION_BWD_FILTER_SPECIFY_WORKSPACE_LIMIT, 0, &m_filterAlgo));

        checkCUDNN(cudnnGetConvolutionBackwardDataWorkspaceSize(this->GetCudnnHandle(), filterDesc, deltaDesc, convDesc, inputDeltaDesc, m_dataAlgo, &m_dataSizeInBytes));

        checkCUDNN(cudnnGetConvolutionBackwardFilterWorkspaceSize(this->GetCudnnHandle(), inputTensorDesc, deltaDesc, convDesc, filterDesc, m_filterAlgo, &m_filterSizeInBytes));

        if (m_sizeInBytes != 0) {
            checkCudaErrors(cudaMalloc(&m_devWorkSpace, m_sizeInBytes));

            if (m_devWorkSpace == NULL) {
                printf("Failed to DEVICE allocation in %s (%s %d)\n", __FUNCTION__, __FILE__, __LINE__);
                exit(-1);
            }
        }

        if (m_dataSizeInBytes != 0) {
            checkCudaErrors(cudaMalloc(&m_dataDevWorkSpace, m_dataSizeInBytes));

            if (m_dataDevWorkSpace == NULL) {
                printf("Failed to DEVICE allocation in %s (%s %d)\n", __FUNCTION__, __FILE__, __LINE__);
                exit(-1);
            }
        }

        if (m_filterSizeInBytes != 0) {
            checkCudaErrors(cudaMalloc(&m_filterDevWorkSpace, m_filterSizeInBytes));

            if (m_filterDevWorkSpace == NULL) {
                printf("Failed to DEVICE allocation in %s (%s %d)\n", __FUNCTION__, __FILE__, __LINE__);
                exit(-1);
            }
        }

        checkCudaErrors(cudaDeviceSynchronize());
    }

#endif  // if __CUDNN__


    void Delete() {
#if __CUDNN__

        if (inputTensorDesc) checkCUDNN(cudnnDestroyTensorDescriptor(inputTensorDesc));
        inputTensorDesc = NULL;

        if (outputTensorDesc) checkCUDNN(cudnnDestroyTensorDescriptor(outputTensorDesc));
        outputTensorDesc = NULL;

        if (deltaDesc) checkCUDNN(cudnnDestroyTensorDescriptor(deltaDesc));
        deltaDesc = NULL;

        if (inputDeltaDesc) checkCUDNN(cudnnDestroyTensorDescriptor(inputDeltaDesc));
        inputDeltaDesc = NULL;

        if (convDesc) checkCUDNN(cudnnDestroyConvolutionDescriptor(convDesc));
        convDesc = NULL;

        if (filterDesc) checkCUDNN(cudnnDestroyFilterDescriptor(filterDesc));
        filterDesc = NULL;

        if (filterDeltaDesc) checkCUDNN(cudnnDestroyFilterDescriptor(filterDeltaDesc));
        filterDeltaDesc = NULL;

        if (m_pDevInput) checkCudaErrors(cudaFree(m_pDevInput));
        m_pDevInput = NULL;

        if (m_pDevOutput) checkCudaErrors(cudaFree(m_pDevOutput));
        m_pDevOutput = NULL;

        if (m_pDevFilter) checkCudaErrors(cudaFree(m_pDevFilter));
        m_pDevFilter = NULL;

        if (m_pDevInputDelta) checkCudaErrors(cudaFree(m_pDevInputDelta));
        m_pDevInputDelta = NULL;

        if (m_pDevDelta) checkCudaErrors(cudaFree(m_pDevDelta));
        m_pDevDelta = NULL;

        if (m_pDevFilterDelta) checkCudaErrors(cudaFree(m_pDevFilterDelta));
        m_pDevFilterDelta = NULL;

        if (m_sizeInBytes != 0) {
            checkCudaErrors(cudaFree(m_devWorkSpace));
        }

        if (m_dataSizeInBytes != 0) {
            checkCudaErrors(cudaFree(m_dataDevWorkSpace));
        }

        if (m_filterSizeInBytes != 0) {
            checkCudaErrors(cudaFree(m_filterDevWorkSpace));
        }

        checkCudaErrors(cudaThreadSynchronize());

#endif  // if __CUDNN__
    }

    int ForwardPropagate(int pTime = 0, int pThreadNum = 0) {
        Tensor<DTYPE> *weight = this->GetInput()[0]->GetResult();
        Tensor<DTYPE> *input  = this->GetInput()[1]->GetResult();
        Tensor<DTYPE> *result = this->GetResult();

        int timesize    = result->GetTimeSize();
        int batchsize   = result->GetBatchSize();
        int channelsize = result->GetChannelSize();
        int rowsize     = result->GetRowSize();
        int colsize     = result->GetColSize();

        int hiddensize = input->GetColSize();

        int weight_index = 0;
        int input_index  = 0;
        int result_index = 0;

        Shape *weightTenShape = weight->GetShape();
        Shape *inputTenShape  = input->GetShape();
        Shape *resultTenShape = result->GetShape();

        int ti = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int ba = pThreadNum; ba < batchsize; ba += numOfThread) {
            for (int ch = 0; ch < channelsize; ch++) {
                for (int ro = 0; ro < rowsize; ro++) {
                    for (int co = 0; co < colsize; co++) {
                        for (int hid = 0; hid < hiddensize; hid++) {
                            (*result)[Index5D(resultTenShape, ti, ba, ch, ro, co)]
                                += (*weight)[Index5D(weightTenShape, 0, 0, 0, co, hid)]
                                   * (*input)[Index5D(inputTenShape, ti, ba, ch, ro, hid)];
                        }
                    }
                }
            }
        }


        return TRUE;
    }

    int BackPropagate(int pTime = 0, int pThreadNum = 0) {
        Tensor<DTYPE> *weight = this->GetInput()[0]->GetResult();
        Tensor<DTYPE> *input  = this->GetInput()[1]->GetResult();

        Tensor<DTYPE> *this_delta      = this->GetDelta();
        Tensor<DTYPE> *weight_gradient = this->GetInput()[0]->GetGradient();
        Tensor<DTYPE> *input_delta     = this->GetInput()[1]->GetDelta();

        int timesize    = this_delta->GetTimeSize();
        int batchsize   = this_delta->GetBatchSize();
        int channelsize = this_delta->GetChannelSize();
        int rowsize     = this_delta->GetRowSize();
        int colsize     = this_delta->GetColSize();
        int hiddensize  = input_delta->GetColSize();

        Shape *weightTenShape = weight->GetShape();
        Shape *inputTenShape  = input->GetShape();
        Shape *resultTenShape = this_delta->GetShape();

        int weight_index = 0;
        int input_index  = 0;
        int result_index = 0;

        int ti = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int ba = pThreadNum; ba < batchsize; ba += numOfThread) {
            for (int ch = 0; ch < channelsize; ch++) {
                for (int ro = 0; ro < rowsize; ro++) {
                    for (int co = 0; co < colsize; co++) {
                        for (int hid = 0; hid < hiddensize; hid++) {
                            weight_index = Index5D(weightTenShape, 0, 0, 0, co, hid);
                            input_index  = Index5D(inputTenShape, ti, ba, ch, ro, hid);
                            result_index = Index5D(resultTenShape, ti, ba, ch, ro, co);

                            (*input_delta)[input_index]      += (*weight)[weight_index] * (*this_delta)[result_index];
                            (*weight_gradient)[weight_index] += (*input)[input_index] * (*this_delta)[result_index];
                        }
                    }
                }
            }
        }


        return TRUE;
    }

#if __CUDNN__
    int ForwardPropagateOnGPU(int pTime = 0) {
        Tensor<DTYPE> *weight = this->GetInput()[0]->GetResult();
        Tensor<DTYPE> *input  = this->GetInput()[1]->GetResult();
        Tensor<DTYPE> *result = this->GetResult();

        int inputCapacity  = input->GetCapacity();
        int outputCapacity = result->GetCapacity();
        int filterCapacity = weight->GetCapacity();

        m_pHostFilter = weight->GetHostData();
        m_pHostInput  = input->GetHostData();
        m_pHostOutput = result->GetHostData();

        checkCudaErrors(cudaMemcpy(m_pDevInput, m_pHostInput, (inputCapacity * sizeof(DTYPE)), cudaMemcpyHostToDevice));
        checkCudaErrors(cudaMemcpy(m_pDevFilter, m_pHostFilter, (filterCapacity * sizeof(DTYPE)), cudaMemcpyHostToDevice));


        checkCUDNN(cudnnConvolutionForward(this->GetCudnnHandle(), &m_alpha, inputTensorDesc, m_pDevInput, filterDesc, m_pDevFilter, convDesc,
                                           m_algo, m_devWorkSpace, m_sizeInBytes, &m_beta, outputTensorDesc, m_pDevOutput));

        checkCudaErrors(cudaMemcpy(m_pHostOutput, m_pDevOutput, (outputCapacity * sizeof(DTYPE)), cudaMemcpyDeviceToHost));

        checkCudaErrors(cudaDeviceSynchronize());

        return TRUE;
    }

    int BackPropagateOnGPU(int pTime = 0) {
        Tensor<DTYPE> *weight          = this->GetInput()[0]->GetResult();
        Tensor<DTYPE> *weight_gradient = this->GetInput()[0]->GetGradient();

        Tensor<DTYPE> *input       = this->GetInput()[1]->GetResult();
        Tensor<DTYPE> *input_delta = this->GetInput()[1]->GetDelta();

        Tensor<DTYPE> *this_delta = this->GetDelta();

        int inputCapacity       = input->GetCapacity();
        int filterCapacity      = weight->GetCapacity();
        int inputDeltaCapacity  = input->GetCapacity();
        int deltaCapacity       = this_delta->GetCapacity();
        int filterDeltaCapacity = weight_gradient->GetCapacity();

        m_pHostFilter      = weight->GetHostData();
        m_pHostInput       = input->GetHostData();
        m_pHostDelta       = this_delta->GetHostData();
        m_pHostFilterDelta = weight_gradient->GetHostData();
        m_pHostInputDelta  = input_delta->GetHostData();

        checkCudaErrors(cudaMemcpy(m_pDevInput, m_pHostInput, (inputCapacity) * sizeof(DTYPE), cudaMemcpyHostToDevice));
        checkCudaErrors(cudaMemcpy(m_pDevFilter, m_pHostFilter, (filterCapacity * sizeof(DTYPE)), cudaMemcpyHostToDevice));
        checkCudaErrors(cudaMemcpy(m_pDevDelta, m_pHostDelta, (deltaCapacity) * sizeof(DTYPE), cudaMemcpyHostToDevice));

        checkCUDNN(cudnnConvolutionBackwardData(this->GetCudnnHandle(), &m_alpha, filterDesc, m_pDevFilter, deltaDesc, m_pDevDelta, convDesc,
                                                m_dataAlgo, m_dataDevWorkSpace, m_dataSizeInBytes, &m_beta, inputDeltaDesc, m_pDevInputDelta));

        checkCUDNN(cudnnConvolutionBackwardFilter(this->GetCudnnHandle(), &m_alpha, inputTensorDesc, m_pDevInput, deltaDesc, m_pDevDelta, convDesc,
                                                  m_filterAlgo, m_filterDevWorkSpace, m_filterSizeInBytes, &m_beta, filterDesc, m_pDevFilterDelta));

        checkCudaErrors(cudaMemcpy(m_pHostInputDelta, m_pDevInputDelta, (inputDeltaCapacity) * sizeof(DTYPE), cudaMemcpyDeviceToHost));
        checkCudaErrors(cudaMemcpy(m_pHostFilterDelta, m_pDevFilterDelta, (filterDeltaCapacity) * sizeof(DTYPE), cudaMemcpyDeviceToHost));

        checkCudaErrors(cudaDeviceSynchronize());
        return TRUE;
    }

#endif  // if __CUDNN__
};


#endif  // MATMUL_H_
