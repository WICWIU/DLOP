#ifndef NEURALNETWORK_H_
#define NEURALNETWORK_H_

#include "Optimizer//GradientDescentOptimizer.h"

template<typename DTYPE> class NeuralNetwork {
private:
#if __CUDNN__
    cudnnHandle_t m_cudnnHandle;
#endif  // if __CUDNN__
    Container<Operator<DTYPE> *> *m_aaOperator;
    Container<Tensorholder<DTYPE> *> *m_aaTensorholder;

    int m_OperatorDegree;
    int m_TensorholderDegree;

    Objective<DTYPE> *m_aObjective;
    Optimizer<DTYPE> *m_aOptimizer;

public:
    NeuralNetwork();
    virtual ~NeuralNetwork();

    int  Alloc();
    void Delete();

    // =======

    // 추후 직접 변수를 만들지 않은 operator* + operator*의 변환 변수도 자동으로 할당될 수 있도록 Operator와 NN class를 수정해야 한다.
    Operator<DTYPE>    * AddOperator(Operator<DTYPE> *pOperator);
    Tensorholder<DTYPE>* AddTensorholder(Tensorholder<DTYPE> *pTensorholder);

    Objective<DTYPE>   * SetObjective(Objective<DTYPE> *pObjective);
    Optimizer<DTYPE>   * SetOptimizer(Optimizer<DTYPE> *pOptimizer);

    // =======

    Operator<DTYPE>                 * GetResultOperator();
    Operator<DTYPE>                 * GetResult();

    Container<Tensorholder<DTYPE> *>* GetTensorholder();

    Objective<DTYPE>                * GetObjective();
    Optimizer<DTYPE>                * GetOptimizer();

    // =======
    float                             GetAccuracy();
    int                               GetMaxIndex(Tensor<DTYPE> *data, int ba, int numOfClass);
    float                             GetLoss();

    // =======
    int                               ForwardPropagate();
    int                               ForwardPropagate(Operator<DTYPE> *pEnd);
    int                               ForwardPropagate(Operator<DTYPE> *pStart, Operator<DTYPE> *pEnd);
    int                               BackPropagate();

    // =======
    int                               Training();
    int                               Testing();

    // =======

    int CreateGraph();

    // reset value
    int ResetOperatorResult();
    int ResetOperatorGradient();

    int ResetObjectiveResult();
    int ResetObjectiveGradient();

    int ResetParameterGradient();
};

#endif  // NEURALNETWORK_H_
