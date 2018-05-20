#ifndef ADD_H_
#define ADD_H_    value

#include "..//Operator.h"

template<typename DTYPE> class Addall : public Operator<DTYPE>{
private:
    Shape *m_pLeftTenShape;
    Shape *m_pRightTenShape;

    int m_timesize;
    int m_batchsize;
    int m_channelsize;
    int m_rowsize;
    int m_colsize;

#if __CUDNN__
    cudnnTensorDescriptor_t leftTensorDesc, rightTensorDesc, outputTensorDesc, leftDeltaDesc, rightDeltaDesc, deltaDesc;
    DTYPE *m_pDevLeft, *m_pDevRight, *m_pDevOutput, *m_pDevLeftDelta, *m_pDevRightDelta, *m_pDevDelta;

    DTYPE m_alpha;
    DTYPE m_beta;

#endif  // __CUDNN__

public:
    Addall(Operator<DTYPE> *pLeftInput, Operator<DTYPE> *pRightInput, std::string pName) : Operator<DTYPE>(pLeftInput, pRightInput, pName) {
        #if __DEBUG__
        std::cout << "Addall::Addall(Operator<DTYPE> *, Operator<DTYPE> *, std::string)" << '\n';
        #endif  // __DEBUG__
        this->Alloc(pLeftInput, pRightInput);
    }

    ~Addall() {
        #if __DEBUG__
        std::cout << "Addall::~Addall()" << '\n';
        #endif  // __DEBUG__
    }

    int Alloc(Operator<DTYPE> *pLeftInput, Operator<DTYPE> *pRightInput) {
        #if __DEBUG__
        std::cout << "Addall::Alloc(Operator<DTYPE> *, Operator<DTYPE> *)" << '\n';
        #endif  // __DEBUG__

        m_pLeftTenShape  = pLeftInput->GetResult()->GetShape();
        m_pRightTenShape = pRightInput->GetResult()->GetShape();

        m_timesize    = (*m_pLeftTenShape)[0];
        m_batchsize   = (*m_pLeftTenShape)[1];
        m_channelsize = (*m_pLeftTenShape)[2];
        m_rowsize     = (*m_pLeftTenShape)[3];
        m_colsize     = (*m_pLeftTenShape)[4];

        this->SetResult(new Tensor<DTYPE>(m_timesize, m_batchsize, m_channelsize, m_rowsize, m_colsize));

        this->SetGradient(new Tensor<DTYPE>(m_timesize, m_batchsize, m_channelsize, m_rowsize, m_colsize));

        return TRUE;
    }

#if __CUDNN__
    void InitializeAttributeForGPU() {
        m_batchsize   = (*m_pLeftTenShape)[1];
        m_channelsize = (*m_pLeftTenShape)[2];
        m_rowsize     = (*m_pLeftTenShape)[3];
        m_colsize     = (*m_pLeftTenShape)[4];

        m_alpha = 1;
        m_beta  = 0;

        checkCUDNN(cudnnCreateTensorDescriptor(&leftTensorDesc));
        checkCUDNN(cudnnCreateTensorDescriptor(&rightTensorDesc));
        checkCUDNN(cudnnCreateTensorDescriptor(&outputTensorDesc));
        checkCUDNN(cudnnCreateTensorDescriptor(&leftDeltaDesc));
        checkCUDNN(cudnnCreateTensorDescriptor(&rightDeltaDesc));
        checkCUDNN(cudnnCreateTensorDescriptor(&deltaDesc));

        checkCUDNN(cudnnSetTensor4dDescriptor(leftTensorDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              m_batchsize, m_channelsize, m_rowsize, m_colsize));

        checkCUDNN(cudnnSetTensor4dDescriptor(rightTensorDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              m_batchsize, m_channelsize, m_rowsize, m_colsize));

        checkCUDNN(cudnnSetTensor4dDescriptor(outputTensorDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              m_batchsize, m_channelsize, m_rowsize, m_colsize));

        checkCUDNN(cudnnSetTensor4dDescriptor(leftDeltaDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              m_batchsize, m_channelsize, m_rowsize, m_colsize));

        checkCUDNN(cudnnSetTensor4dDescriptor(rightDeltaDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              m_batchsize, m_channelsize, m_rowsize, m_colsize));

        checkCUDNN(cudnnSetTensor4dDescriptor(deltaDesc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT,
                                              m_batchsize, m_channelsize, m_rowsize, m_colsize));

        checkCudaErrors(cudaDeviceSynchronize());
    }

#endif  // if __CUDNN__

    void Delete() {
#if __CUDNN__

        if (leftTensorDesc) checkCUDNN(cudnnDestroyTensorDescriptor(leftTensorDesc));
        leftTensorDesc = NULL;

        if (rightTensorDesc) checkCUDNN(cudnnDestroyTensorDescriptor(rightTensorDesc));
        rightTensorDesc = NULL;

        if (outputTensorDesc) checkCUDNN(cudnnDestroyTensorDescriptor(outputTensorDesc));
        outputTensorDesc = NULL;

        if (leftDeltaDesc) checkCUDNN(cudnnDestroyTensorDescriptor(leftDeltaDesc));
        leftDeltaDesc = NULL;

        if (rightDeltaDesc) checkCUDNN(cudnnDestroyTensorDescriptor(rightDeltaDesc));
        rightDeltaDesc = NULL;

        if (deltaDesc) checkCUDNN(cudnnDestroyTensorDescriptor(deltaDesc));
        deltaDesc = NULL;

        checkCudaErrors(cudaThreadSynchronize());
#endif  // if __CUDNN__
    }

    int ForwardPropagate(int pTime = 0, int pThreadNum = 0) {
        Container<Operator<DTYPE> *> *input_contatiner = this->GetInputContainer();

        Tensor<DTYPE> *left   = (*input_contatiner)[0]->GetResult();
        Tensor<DTYPE> *right  = (*input_contatiner)[1]->GetResult();
        Tensor<DTYPE> *result = this->GetResult();

        int m_ti        = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int m_ba = pThreadNum; m_ba < m_batchsize; m_ba += numOfThread) {
            for (int m_ch = 0; m_ch < m_channelsize; m_ch++) {
                for (int m_ro = 0; m_ro < m_rowsize; m_ro++) {
                    for (int m_co = 0; m_co < m_colsize; m_co++) {
                        (*result)[Index5D(m_pLeftTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                            = (*left)[Index5D(m_pLeftTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                              + (*right)[Index5D(m_pRightTenShape, m_ti, m_ba, m_ch, m_ro, m_co)];
                    }
                }
            }
        }

        return TRUE;
    }

    int BackPropagate(int pTime = 0, int pThreadNum = 0) {
        Container<Operator<DTYPE> *> *input_contatiner = this->GetInputContainer();

        Tensor<DTYPE> *left_grad  = (*input_contatiner)[0]->GetGradient();
        Tensor<DTYPE> *right_grad = (*input_contatiner)[1]->GetGradient();
        Tensor<DTYPE> *this_grad  = this->GetGradient();

        int m_ti        = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int m_ba = pThreadNum; m_ba < m_batchsize; m_ba += numOfThread) {
            for (int m_ch = 0; m_ch < m_channelsize; m_ch++) {
                for (int m_ro = 0; m_ro < m_rowsize; m_ro++) {
                    for (int m_co = 0; m_co < m_colsize; m_co++) {
                        (*left_grad)[Index5D(m_pLeftTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                            += (*this_grad)[Index5D(m_pLeftTenShape, m_ti, m_ba, m_ch, m_ro, m_co)];

                        (*right_grad)[Index5D(m_pRightTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                            += (*this_grad)[Index5D(m_pLeftTenShape, m_ti, m_ba, m_ch, m_ro, m_co)];
                    }
                }
            }
        }

        return TRUE;
    }

#if __CUDNN__
    int ForwardPropagateOnGPU(int pTime) {
        Container<Operator<DTYPE> *> *input_contatiner = this->GetInputContainer();

        Tensor<DTYPE> *left   = (*input_contatiner)[0]->GetResult();
        Tensor<DTYPE> *right  = (*input_contatiner)[1]->GetResult();
        Tensor<DTYPE> *result = this->GetResult();

        m_pDevLeft   = left->GetDeviceData(pTime);
        m_pDevRight  = right->GetDeviceData(pTime);
        m_pDevOutput = result->GetDeviceData(pTime);

        checkCUDNN(cudnnAddTensor(this->GetCudnnHandle(),
                                  &m_alpha, leftTensorDesc, m_pDevLeft,
                                  &m_beta, outputTensorDesc, m_pDevOutput));

        checkCUDNN(cudnnAddTensor(this->GetCudnnHandle(),
                                  &m_alpha, rightTensorDesc, m_pDevRight,
                                  &m_alpha, outputTensorDesc, m_pDevOutput));

        // this->ForwardPropagate();
        return TRUE;
    }

    int BackPropagateOnGPU(int pTime) {
        Container<Operator<DTYPE> *> *input_contatiner = this->GetInputContainer();

        Tensor<DTYPE> *left_grad  = (*input_contatiner)[0]->GetGradient();
        Tensor<DTYPE> *right_grad = (*input_contatiner)[1]->GetGradient();
        Tensor<DTYPE> *this_grad  = this->GetGradient();

        m_pDevLeftDelta  = left_grad->GetDeviceData(pTime);
        m_pDevRightDelta = right_grad->GetDeviceData(pTime);
        m_pDevDelta      = this_grad->GetDeviceData(pTime);

        checkCUDNN(cudnnAddTensor(this->GetCudnnHandle(),
                                  &m_alpha, deltaDesc, m_pDevDelta,
                                  &m_beta, leftDeltaDesc, m_pDevLeftDelta));

        checkCUDNN(cudnnAddTensor(this->GetCudnnHandle(),
                                  &m_alpha, deltaDesc, m_pDevDelta,
                                  &m_beta, rightDeltaDesc, m_pDevRightDelta));

        // this->BackPropagate();

        return TRUE;
    }

#endif  // __CUDNN__
};


template<typename DTYPE> class AddColWise : public Operator<DTYPE>{
private:
    Shape *m_pInputTenShape;
    Shape *m_pBiasTenShape;

    int m_timesize;
    int m_batchsize;
    int m_channelsize;
    int m_rowsize;
    int m_colsize;

public:
    AddColWise(Operator<DTYPE> *pInput, Operator<DTYPE> *pBias, std::string pName) : Operator<DTYPE>(pInput, pBias, pName) {
        #if __DEBUG__
        std::cout << "AddColWise::AddColWise(Operator<DTYPE> *, Operator<DTYPE> *, std::string)" << '\n';
        #endif  // __DEBUG__
        this->Alloc(pInput, pBias);
    }

    ~AddColWise() {
        #if __DEBUG__
        std::cout << "AddColWise::~AddColWise()" << '\n';
        #endif  // __DEBUG__
    }

    int Alloc(Operator<DTYPE> *pInput, Operator<DTYPE> *pBias) {
        #if __DEBUG__
        std::cout << "AddColWise::Alloc(Operator<DTYPE> *, Operator<DTYPE> *)" << '\n';
        #endif  // __DEBUG__

        m_pInputTenShape = pInput->GetResult()->GetShape();
        m_pBiasTenShape  = pBias->GetResult()->GetShape();

        m_timesize    = (*m_pInputTenShape)[0];
        m_batchsize   = (*m_pInputTenShape)[1];
        m_channelsize = (*m_pInputTenShape)[2];
        m_rowsize     = (*m_pInputTenShape)[3];
        m_colsize     = (*m_pInputTenShape)[4];

        #if __DEBUG__

        if ((*m_pBiasTenShape)[0] != 1) printf("Receive invalid bias shape in %s (%s %d), cannot handling\n", __FUNCTION__, __FILE__, __LINE__);

        if ((*m_pBiasTenShape)[1] != 1) printf("Receive invalid bias shape in %s (%s %d), cannot handling\n", __FUNCTION__, __FILE__, __LINE__);

        if ((*m_pBiasTenShape)[2] != 1) printf("Receive invalid bias shape in %s (%s %d), cannot handling\n", __FUNCTION__, __FILE__, __LINE__);

        if ((*m_pBiasTenShape)[3] != 1) printf("Receive invalid bias shape in %s (%s %d), cannot handling\n", __FUNCTION__, __FILE__, __LINE__);
        #endif  // __DEBUG__

        this->SetResult(new Tensor<DTYPE>(m_timesize, m_batchsize, m_channelsize, m_rowsize, m_colsize));

        this->SetGradient(new Tensor<DTYPE>(m_timesize, m_batchsize, m_channelsize, m_rowsize, m_colsize));

        return TRUE;
    }

    int ForwardPropagate(int pTime = 0, int pThreadNum = 0) {
        Container<Operator<DTYPE> *> *input_contatiner = this->GetInputContainer();

        Tensor<DTYPE> *input  = (*input_contatiner)[0]->GetResult();
        Tensor<DTYPE> *bias   = (*input_contatiner)[1]->GetResult();
        Tensor<DTYPE> *result = this->GetResult();

        int m_ti        = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int m_ba = pThreadNum; m_ba < m_batchsize; m_ba += numOfThread) {
            for (int m_ch = 0; m_ch < m_channelsize; m_ch++) {
                for (int m_ro = 0; m_ro < m_rowsize; m_ro++) {
                    for (int m_co = 0; m_co < m_colsize; m_co++) {
                        (*result)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                            = (*input)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                              + (*bias)[m_co];
                    }
                }
            }
        }


        return TRUE;
    }

    int BackPropagate(int pTime = 0, int pThreadNum = 0) {
        Container<Operator<DTYPE> *> *input_contatiner = this->GetInputContainer();

        Tensor<DTYPE> *input_grad = (*input_contatiner)[0]->GetGradient();
        Tensor<DTYPE> *bias_grad  = (*input_contatiner)[1]->GetGradient();
        Tensor<DTYPE> *this_grad  = this->GetGradient();

        int m_ti        = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int m_ba = pThreadNum; m_ba < m_batchsize; m_ba += numOfThread) {
            for (int m_ch = 0; m_ch < m_channelsize; m_ch++) {
                for (int m_ro = 0; m_ro < m_rowsize; m_ro++) {
                    for (int m_co = 0; m_co < m_colsize; m_co++) {
                        (*input_grad)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                            += (*this_grad)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)];

                        (*bias_grad)[m_co]
                            += (*this_grad)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)];
                    }
                }
            }
        }


        return TRUE;
    }

#if __CUDNN__
    int ForwardPropagateOnGPU(int pTime) {
        this->ForwardPropagate();
        return TRUE;
    }

    int BackPropagateOnGPU(int pTime) {
        this->BackPropagate();

        return TRUE;
    }

#endif  // __CUDNN__
};

template<typename DTYPE> class AddChannelWise : public Operator<DTYPE>{
private:
    Shape *m_pInputTenShape;
    Shape *m_pBiasTenShape;

    int m_timesize;
    int m_batchsize;
    int m_channelsize;
    int m_rowsize;
    int m_colsize;

public:
    AddChannelWise(Operator<DTYPE> *pInput, Operator<DTYPE> *pBias, std::string pName) : Operator<DTYPE>(pInput, pBias, pName) {
        #if __DEBUG__
        std::cout << "AddChannelWise::AddChannelWise(Operator<DTYPE> *, Operator<DTYPE> *, std::string)" << '\n';
        #endif  // __DEBUG__
        this->Alloc(pInput, pBias);
    }

    ~AddChannelWise() {
        #if __DEBUG__
        std::cout << "AddChannelWise::~AddChannelWise()" << '\n';
        #endif  // __DEBUG__
    }

    int Alloc(Operator<DTYPE> *pInput, Operator<DTYPE> *pBias) {
        #if __DEBUG__
        std::cout << "AddColWise::Alloc(Operator<DTYPE> *, Operator<DTYPE> *)" << '\n';
        #endif  // __DEBUG__

        m_pInputTenShape = pInput->GetResult()->GetShape();
        m_pBiasTenShape  = pBias->GetResult()->GetShape();

        m_timesize    = (*m_pInputTenShape)[0];
        m_batchsize   = (*m_pInputTenShape)[1];
        m_channelsize = (*m_pInputTenShape)[2];
        m_rowsize     = (*m_pInputTenShape)[3];
        m_colsize     = (*m_pInputTenShape)[4];

        #if __DEBUG__

        if ((*m_pBiasTenShape)[0] != 1) printf("Receive invalid bias shape in %s (%s %d), cannot handling\n", __FUNCTION__, __FILE__, __LINE__);

        if ((*m_pBiasTenShape)[1] != 1) printf("Receive invalid bias shape in %s (%s %d), cannot handling\n", __FUNCTION__, __FILE__, __LINE__);

        if ((*m_pBiasTenShape)[3] != 1) printf("Receive invalid bias shape in %s (%s %d), cannot handling\n", __FUNCTION__, __FILE__, __LINE__);

        if ((*m_pBiasTenShape)[4] != 1) printf("Receive invalid bias shape in %s (%s %d), cannot handling\n", __FUNCTION__, __FILE__, __LINE__);
        #endif  // __DEBUG__

        this->SetResult(new Tensor<DTYPE>(m_timesize, m_batchsize, m_channelsize, m_rowsize, m_colsize));

        this->SetGradient(new Tensor<DTYPE>(m_timesize, m_batchsize, m_channelsize, m_rowsize, m_colsize));

        return TRUE;
    }

    int ForwardPropagate(int pTime = 0, int pThreadNum = 0) {
        Container<Operator<DTYPE> *> *input_contatiner = this->GetInputContainer();

        Tensor<DTYPE> *input  = (*input_contatiner)[0]->GetResult();
        Tensor<DTYPE> *bias   = (*input_contatiner)[1]->GetResult();
        Tensor<DTYPE> *result = this->GetResult();

        int m_ti        = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int m_ba = pThreadNum; m_ba < m_batchsize; m_ba += numOfThread) {
            for (int m_ch = 0; m_ch < m_channelsize; m_ch++) {
                for (int m_ro = 0; m_ro < m_rowsize; m_ro++) {
                    for (int m_co = 0; m_co < m_colsize; m_co++) {
                        (*result)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                            = (*input)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                              + (*bias)[m_ch];
                    }
                }
            }
        }


        return TRUE;
    }

    int BackPropagate(int pTime = 0, int pThreadNum = 0) {
        Container<Operator<DTYPE> *> *input_contatiner = this->GetInputContainer();

        Tensor<DTYPE> *input_grad = (*input_contatiner)[0]->GetGradient();
        Tensor<DTYPE> *bias_grad  = (*input_contatiner)[1]->GetGradient();
        Tensor<DTYPE> *this_grad  = this->GetGradient();

        int m_ti        = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int m_ba = pThreadNum; m_ba < m_batchsize; m_ba += numOfThread) {
            for (int m_ch = 0; m_ch < m_channelsize; m_ch++) {
                for (int m_ro = 0; m_ro < m_rowsize; m_ro++) {
                    for (int m_co = 0; m_co < m_colsize; m_co++) {
                        (*input_grad)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)]
                            += (*this_grad)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)];

                        (*bias_grad)[m_ch]
                            += (*this_grad)[Index5D(m_pInputTenShape, m_ti, m_ba, m_ch, m_ro, m_co)];
                    }
                }
            }
        }

        return TRUE;
    }

#if __CUDNN__
    int ForwardPropagateOnGPU(int pTime) {
        this->ForwardPropagate(pTime);
        return TRUE;
    }

    int BackPropagateOnGPU(int pTime) {
        this->BackPropagate(pTime);

        return TRUE;
    }

#endif  // __CUDNN__
};


#endif  // ADD_H_
