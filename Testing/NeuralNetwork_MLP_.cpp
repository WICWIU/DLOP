#include <iostream>
#include <string>

#include "..//Header//NeuralNetwork.h"

#define BATCH    4

int main(int argc, char const *argv[]) {
    std::cout << "---------------Start-----------------" << '\n';

    NeuralNetwork HGUNN;

    // create input data
    Tensor   *_x1 = Tensor::Constants(1, BATCH, 1, 1, 2, 1.0);
    Operator *x1  = HGUNN.AddPlaceholder(_x1, "x1");
    x1->GetOutput()->GetData()[0][0][0][0][0] = 0;
    x1->GetOutput()->GetData()[0][0][0][0][1] = 0;
    x1->GetOutput()->GetData()[0][1][0][0][0] = 1;
    x1->GetOutput()->GetData()[0][1][0][0][1] = 0;
    x1->GetOutput()->GetData()[0][2][0][0][0] = 0;
    x1->GetOutput()->GetData()[0][2][0][0][1] = 1;
    x1->GetOutput()->GetData()[0][3][0][0][0] = 1;
    x1->GetOutput()->GetData()[0][3][0][0][1] = 1;

    // create label data
    Tensor   *_ans = Tensor::Constants(1, BATCH, 1, 1, 2, 1.0);
    Operator *ans  = HGUNN.AddPlaceholder(_ans, "answer");
    ans->GetOutput()->GetData()[0][0][0][0][0] = 1;
    ans->GetOutput()->GetData()[0][0][0][0][1] = 0;
    ans->GetOutput()->GetData()[0][1][0][0][0] = 0;
    ans->GetOutput()->GetData()[0][1][0][0][1] = 1;
    ans->GetOutput()->GetData()[0][2][0][0][0] = 0;
    ans->GetOutput()->GetData()[0][2][0][0][1] = 1;
    ans->GetOutput()->GetData()[0][3][0][0][0] = 1;
    ans->GetOutput()->GetData()[0][3][0][0][1] = 0;

    // ======================= layer 1=======================
    Tensor   *_w1 = Tensor::Truncated_normal(1, 1, 1, 2, 4, 0.0, 0.6);
    Operator *w1  = new Variable(_w1, "w1", 1);

    Tensor   *_b1 = Tensor::Constants(1, 1, 1, 1, 4, 1.0);
    Operator *b1  = new Variable(_b1, "b1", 1); // 오류 발생 원인 찾기

    Operator *mat_1 = new MatMul(x1, w1, "mat_1");

    Operator *add_1 = new Add(mat_1, b1, "add_1");

    // Operator *act_1 = new Relu(add_1, "relu_1");
    Operator *act_1 = new Sigmoid(add_1, "sig_1");

    // ======================= layer 2=======================
    Tensor   *_w2 = Tensor::Truncated_normal(1, 1, 1, 4, 2, 0.0, 0.6);
    Operator *w2  = new Variable(_w2, "w2", 1);

    Tensor   *_b2 = Tensor::Constants(1, 1, 1, 1, 2, 1.0);
    Operator *b2  = new Variable(_b2, "b2", 1); // 오류 발생 원인 찾기

    Operator *mat_2 = new MatMul(act_1, w2, "mat_2");

    Operator *add_2 = new Add(mat_2, b2, "add_2");

    // Operator *act_2 = new Relu(add_2, "relu_2");
    Operator *act_2 = new Sigmoid(add_2, "sig_2");

    Operator *err = new MSE(act_2, ans, "MSE");

    Optimizer *optimizer = new StochasticGradientDescent(0.6);

    // ======================= Create Graph =======================

    HGUNN.CreateGraph(optimizer, err);

    // ======================= Training =======================

    if (argc != 2) {
        std::cout << "There is no count of training" << '\n';
        return 0;
    }

    for (int i = 0; i < atoi(argv[1]); i++) {
        std::cout << "epoch : " << i << '\n';


        HGUNN.Training();
        HGUNN.UpdateWeight();
    }

    // ======================= Testing =======================

    for (int i = 0; i < 1; i++) {
        std::cout << "input : " << i << '\n';


        HGUNN.Testing();
    }


    std::cout << "---------------End-----------------" << '\n';
    return 0;
}
