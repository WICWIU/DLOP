#ifndef SOFTMAXCROSSENTROPY_H_
#define SOFTMAXCROSSENTROPY_H_    value

#include "..//LossFunction.h"

template<typename DTYPE>
class SoftmaxCrossEntropy : public LossFunction<DTYPE>{
private:
    Tensor<DTYPE> *m_aSoftmaxResult;
    DTYPE m_epsilon;  // for backprop

    int m_timesize;

    DTYPE **sum;
    DTYPE **max;

public:
    SoftmaxCrossEntropy(Operator<DTYPE> *pOperator, Operator<DTYPE> *pLabel, DTYPE epsilon, std::string pName = "NO NAME") : LossFunction<DTYPE>(pOperator, pLabel, pName) {
        #if __DEBUG__
        std::cout << "SoftmaxCrossEntropy::SoftmaxCrossEntropy(Operator<DTYPE> *, Operator<DTYPE> *, int)" << '\n';
        #endif  // __DEBUG__
        Alloc(pOperator, epsilon);
    }

    SoftmaxCrossEntropy(Operator<DTYPE> *pOperator, Operator<DTYPE> *pLabel, std::string pName = "NO NAME") : LossFunction<DTYPE>(pOperator, pLabel, pName) {
        #if __DEBUG__
        std::cout << "SoftmaxCrossEntropy::SoftmaxCrossEntropy(Operator<DTYPE> *, Operator<DTYPE> *, int)" << '\n';
        #endif  // __DEBUG__
        Alloc(pOperator, 1e-6f);
    }

    virtual ~SoftmaxCrossEntropy() {
        #if __DEBUG__
        std::cout << "SoftmaxCrossEntropy::~SoftmaxCrossEntropy()" << '\n';
        #endif  // __DEBUG__
        Delete();
    }

    virtual int Alloc(Operator<DTYPE> *pOperator, DTYPE epsilon) {
        #if __DEBUG__
        std::cout << "SoftmaxCrossEntropy::Alloc(Operator<DTYPE> *, Operator<DTYPE> *, int)" << '\n';
        #endif  // __DEBUG__

        Operator<DTYPE> *pInput = pOperator;

        int timesize    = pInput->GetResult()->GetTimeSize();
        int batchsize   = pInput->GetResult()->GetBatchSize();
        int channelsize = pInput->GetResult()->GetChannelSize();
        int rowsize     = pInput->GetResult()->GetRowSize();
        int colsize     = pInput->GetResult()->GetColSize();

        m_timesize = timesize;

        sum = new DTYPE *[timesize];
        max = new DTYPE *[timesize];

        for (int i = 0; i < timesize; i++) {
            sum[i] = new DTYPE[batchsize];
            max[i] = new DTYPE[batchsize];
        }

        this->SetResult(new Tensor<DTYPE>(timesize, batchsize, 1, 1, 1));

        m_aSoftmaxResult = new Tensor<DTYPE>(timesize, batchsize, channelsize, rowsize, colsize);

        this->SetGradient(new Tensor<DTYPE>(timesize, batchsize, channelsize, rowsize, colsize));

        m_epsilon = epsilon;

        return TRUE;
    }

    virtual void Delete() {
        if (m_aSoftmaxResult) {
            delete m_aSoftmaxResult;
            m_aSoftmaxResult = NULL;
        }

        if (sum) {
            for (int i = 0; i < m_timesize; i++) {
                delete[] sum[i];
                sum[i] = NULL;
            }
            delete[] sum;
        }

        if (max) {
            for (int i = 0; i < m_timesize; i++) {
                delete[] max[i];
                max[i] = NULL;
            }
            delete[] max;
        }
    }

    virtual Tensor<DTYPE>* ForwardPropagate() {
        // 추가로  backprop을 계속해서 구성해나가게 되면, 진행하는 것이 가능하다. label 값을 따로 저장하는 작업이 필요가 없어진다.

        Tensor<DTYPE> *input         = this->GetTensor();
        Tensor<DTYPE> *label         = this->GetLabel()->GetResult();
        Tensor<DTYPE> *softmaxresult = m_aSoftmaxResult;
        Tensor<DTYPE> *result        = this->GetResult();
        Tensor<DTYPE> *gradient      = this->GetGradient();
        // result->Reset();
        // gradient->Reset();

        int timesize    = input->GetTimeSize();
        int batchsize   = input->GetBatchSize();
        int channelsize = input->GetChannelSize();
        int rowsize     = input->GetRowSize();
        int colsize     = input->GetColSize();

        // DTYPE sum[timesize][batchsize] = { 0.f, };
        // DTYPE max[timesize][batchsize] = { 0.f, };
        for (int ti = 0; ti < timesize; ti++) {
            for (int ba = 0; ba < batchsize; ba++) {  // thread
                sum[ti][ba] = 0.f;
                max[ti][ba] = 0.f;
            }
        }

        int numOfOutputDim = 0;

        int count    = timesize * batchsize;
        int capacity = colsize;

        int start = 0;
        int end   = 0;

        for (int ti = 0; ti < timesize; ti++) {
            for (int ba = 0; ba < batchsize; ba++) {
                start = (ti * batchsize + ba) * capacity;
                end   = start + capacity;

                max[ti][ba] = Max(input, start, end);
            }
        }

        DTYPE temp = 0.f;

        for (int ti = 0; ti < timesize; ti++) {
            for (int ba = 0; ba < batchsize; ba++) {
                start = (ti * batchsize + ba) * capacity;
                end   = start + capacity;

                for (int i = start; i < end; i++) {
                    temp += (exp((*input)[i] - max[ti][ba]) + m_epsilon);
                }
                sum[ti][ba] = temp;
                temp        = 0.f;
            }
        }

        for (int ti = 0; ti < timesize; ti++) {
            for (int ba = 0; ba < batchsize; ba++) {
                start = (ti * batchsize + ba) * capacity;
                end   = start + capacity;

                for (int i = start; i < end; i++) {
                    (*softmaxresult)[i] = (exp((*input)[i] - max[ti][ba]) + m_epsilon) / sum[ti][ba];

                    (*result)[ti * batchsize + ba] += -(*label)[i] * log((*softmaxresult)[i] + m_epsilon);

                    (*gradient)[i] = (*softmaxresult)[i] - (*label)[i];
                }
            }
        }


        return result;
    }

    virtual Tensor<DTYPE>* BackPropagate() {
        Tensor<DTYPE> *gradient = this->GetGradient();

        Tensor<DTYPE> *softmaxresult = m_aSoftmaxResult;

        Tensor<DTYPE> *input_delta = this->GetOperator()->GetDelta();

        int timesize  = gradient->GetTimeSize();
        int batchsize = gradient->GetBatchSize();
        int colsize   = gradient->GetColSize();

        int count    = timesize * batchsize;
        int capacity = colsize;

        int start = 0;
        int end   = 0;

        for (int ti = 0; ti < timesize; ti++) {
            for (int ba = 0; ba < batchsize; ba++) {
                start = (ti * batchsize + ba) * capacity;
                end   = start + capacity;

                for (int i = start; i < end; i++) {
                    (*input_delta)[i] = (*gradient)[i] / batchsize;
                }
            }
        }

        // int capacity = input_delta->GetCapacity();
        //
        // for (int i = 0; i < capacity; i++) {
        // (*input_delta)[i] = (*gradient)[i] / batchsize;
        // }

        return NULL;
    }

    virtual Tensor<DTYPE>* ForwardPropagate(int pTime, int pThreadNum) {
        // 추가로  backprop을 계속해서 구성해나가게 되면, 진행하는 것이 가능하다. label 값을 따로 저장하는 작업이 필요가 없어진다.

        Tensor<DTYPE> *input         = this->GetTensor();
        Tensor<DTYPE> *label         = this->GetLabel()->GetResult();
        Tensor<DTYPE> *softmaxresult = m_aSoftmaxResult;
        Tensor<DTYPE> *result        = this->GetResult();
        Tensor<DTYPE> *gradient      = this->GetGradient();
        // result->Reset();
        // gradient->Reset();

        int timesize    = input->GetTimeSize();
        int batchsize   = input->GetBatchSize();
        int channelsize = input->GetChannelSize();
        int rowsize     = input->GetRowSize();
        int colsize     = input->GetColSize();

        int ti          = pTime;
        int numOfThread = this->GetNumOfThread();

        // DTYPE sum[timesize][batchsize] = { 0.f, };
        // DTYPE max[timesize][batchsize] = { 0.f, };
        for (int ba = pThreadNum; ba < batchsize; ba += numOfThread) {  // thread
            sum[ti][ba] = 0.f;
            max[ti][ba] = 0.f;
        }

        int numOfOutputDim = 0;

        int count    = timesize * batchsize;
        int capacity = colsize;

        int start = 0;
        int end   = 0;

        for (int ba = pThreadNum; ba < batchsize; ba += numOfThread) {
            start = (ti * batchsize + ba) * capacity;
            end   = start + capacity;

            max[ti][ba] = Max(input, start, end);
        }

        DTYPE temp = 0.f;

        for (int ba = pThreadNum; ba < batchsize; ba += numOfThread) {
            start = (ti * batchsize + ba) * capacity;
            end   = start + capacity;

            for (int i = start; i < end; i++) {
                temp += (exp((*input)[i] - max[ti][ba]) + m_epsilon);
            }
            sum[ti][ba] = temp;
            temp        = 0.f;
        }

        for (int ba = pThreadNum; ba < batchsize; ba += numOfThread) {
            start = (ti * batchsize + ba) * capacity;
            end   = start + capacity;

            for (int i = start; i < end; i++) {
                (*softmaxresult)[i] = (exp((*input)[i] - max[ti][ba]) + m_epsilon) / sum[ti][ba];

                (*result)[ti * batchsize + ba] += -(*label)[i] * log((*softmaxresult)[i] + m_epsilon);

                (*gradient)[i] = (*softmaxresult)[i] - (*label)[i];
            }
        }


        return result;
    }

    virtual Tensor<DTYPE>* BackPropagate(int pTime, int pThreadNum) {
        Tensor<DTYPE> *gradient = this->GetGradient();

        Tensor<DTYPE> *softmaxresult = m_aSoftmaxResult;

        Tensor<DTYPE> *input_delta = this->GetOperator()->GetDelta();

        int timesize  = gradient->GetTimeSize();
        int batchsize = gradient->GetBatchSize();
        int colsize   = gradient->GetColSize();

        int count    = timesize * batchsize;
        int capacity = colsize;

        int start = 0;
        int end   = 0;

        int ti          = pTime;
        int numOfThread = this->GetNumOfThread();

        for (int ba = pThreadNum; ba < batchsize; ba += numOfThread) {
            start = (ti * batchsize + ba) * capacity;
            end   = start + capacity;

            for (int i = start; i < end; i++) {
                (*input_delta)[i] = (*gradient)[i] / batchsize;
            }
        }

        // int capacity = input_delta->GetCapacity();
        //
        // for (int i = 0; i < capacity; i++) {
        // (*input_delta)[i] = (*gradient)[i] / batchsize;
        // }

        return NULL;
    }

    DTYPE Max(Tensor<DTYPE> *input, int start, int end) {
        DTYPE max = (*input)[start];

        for (int i = start + 1; i < end; i++) {
            if ((*input)[i] > max) max = (*input)[i];
        }

        return max;
    }
};

#endif  // SOFTMAXCROSSENTROPY_H_