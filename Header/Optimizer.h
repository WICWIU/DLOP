#ifndef OPTIMIZER_H_
#define OPTIMIZER_H_    value

#include "Objective//SoftmaxCrossEntropy.h"
#include "Objective//MSE.h"

enum OptimizeDirection {
    MAXIMIZE,
    MINIMIZE
};

template<typename DTYPE> class Optimizer {
private:
    float m_LearningRate;
    int m_OptimizeDirection;  // 1 or -1

    Container<Tensorholder<DTYPE> *> *m_ppTrainableTensors;
    int m_TrainableTensorDegree;

public:
    Optimizer(Operator<DTYPE> **pTrainableTensors, float pLearningRate, OptimizeDirection pOptimizeDirection);
    Optimizer(Container<Tensorholder<DTYPE> *> *pTrainableTensors, float pLearningRate, OptimizeDirection pOptimizeDirection);


    virtual ~Optimizer();

    // ===============

    int Alloc(Container<Tensorholder<DTYPE> *> *pTrainableTensors, float pLearningRate, OptimizeDirection pOptimizeDirection);

    int Delete();

    // int AddTrainableTensor(Operator<DTYPE> **pTrainableTensors);
    // int AddTrainableTensor(Operator<DTYPE> *pTrainableTensor);

    // ===============
    int         UpdateVariable();

    // virtual int UpdateVariable(Tensor<DTYPE> *Trainable, Tensor<DTYPE> *Gradient) = 0;
    virtual int UpdateVariable(Tensorholder<DTYPE> *pTrainableTensor) = 0;

    // ===============

    void  SetLearningRate(float pLearningRate);

    void  SetTrainableTensorDegree(int pTrainableTensorDegree);

    float GetLearningRate() const;

    int   GetOptimizeDirection() const;

    int   ResetParameterGradient();
};

#endif  // OPTIMIZER_H_
