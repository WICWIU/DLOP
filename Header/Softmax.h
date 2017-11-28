#ifndef SOFTMAX_H_
#define SOFTMAX_H_    value

#include <iostream>
#include <string>

#include "Tensor.h"
#include "Operator.h"

class Softmax : public Operator {
public:
    // Constructor의 작업 순서는 다음과 같다.
    // 상속을 받는 Operator(Parent class)의 Alloc()을 실행하고, (Operator::Alloc())
    // 나머지 MetaParameter에 대한 Alloc()을 진행한다. (Softmax::Alloc())
    Softmax(Operator *pInput, std::string pName) : Operator(pInput, pName) {
        std::cout << "Softmax::Softmax(Operator *)" << '\n';
        Alloc(pInput);
    }

    virtual ~Softmax() {
        std::cout << "Softmax::~Softmax()" << '\n';
    }

    virtual bool Alloc(Operator *pInput) {
        std::cout << "Softmax::Alloc(Operator *, Operator *)" << '\n';

        Tensor *output = new Tensor(GetInputOperator()[0]->GetOutput()->GetShape());
        SetOutput(output);
        Tensor *delta = new Tensor(GetInputOperator()[0]->GetOutput()->GetShape());
        SetDelta(delta);

        return true;
    }

    virtual bool ComputeForwardPropagate() {
        // std::cout << GetName() << " : ComputeForwardPropagate()" << '\n';

        int *shape         = GetInputOperator()[0]->GetOutput()->GetShape();
        double *****input  = GetInputOperator()[0]->GetOutput()->GetData();
        double *****output = GetOutput()->GetData();

        int Time    = shape[0];
        int Batch   = shape[1];
        int Channel = shape[2];
        int Row     = shape[3];
        int Col     = shape[4];

        double sum[Time][Batch] = {0.0};

        for (int ti = 0; ti < Time; ti++) {
            for (int ba = 0; ba < Batch; ba++) {
                for (int ch = 0; ch < Channel; ch++) {
                    for (int ro = 0; ro < Row; ro++) {
                        for (int co = 0; co < Col; co++) {
                            sum[ti][ba] += input[ti][ba][ch][ro][co];
                        }
                    }
                }
            }
        }

        for (int ti = 0; ti < Time; ti++) {
            for (int ba = 0; ba < Batch; ba++) {
                for (int ch = 0; ch < Channel; ch++) {
                    for (int ro = 0; ro < Row; ro++) {
                        for (int co = 0; co < Col; co++) {
                            output[ti][ba][ch][ro][co] = input[ti][ba][ch][ro][co] / sum[ti][ba];
                        }
                    }
                }
            }
        }

        return true;
    }

    virtual bool ComputeBackPropagate() {
        // std::cout << GetName() << " : ComputeBackPropagate()" << '\n';

        int *shape              = GetOutput()->GetShape();
        double *****output      = GetOutput()->GetData();
        double *****delta       = GetDelta()->GetData();
        double *****delta_input = GetInputOperator()[0]->GetDelta()->GetData();

        for (int ti = 0; ti < shape[0]; ti++) {
            for (int ba = 0; ba < shape[1]; ba++) {
                for (int ch = 0; ch < shape[2]; ch++) {
                    for (int ro = 0; ro < shape[3]; ro++) {
                        for (int co = 0; co < shape[4]; co++) {
                            delta_input[ti][ba][ch][ro][co] = delta[ti][ba][ch][ro][co]
                                                              * output[ti][ba][ch][ro][co]
                                                              * (1 - output[ti][ba][ch][ro][co]);
                        }
                    }
                }
            }
        }

        // GetInputOperator()[0]->GetDelta()->PrintData();

        GetDelta()->Reset();

        return true;
    }

    // for Softmax
    double softmax(double data) {
        return 1.F / (1.F + (double)exp(-data));
    }
};

#endif  // SOFTMAX_H_
