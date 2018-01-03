/*g++ -g -o testing -std=c++11 Convolution_With_MNIST.cpp ../Header/Shape.cpp ../Header/Data.cpp ../Header/Tensor.cpp ../Header/Operator.cpp ../Header/Optimizer.cpp*/

#include <iostream>
#include <string>

#include "..//Header//NeuralNetwork.h"
// #include "..//Header//DLOP.h"
#include "..//Header//Temporary_method.h"
#include "MNIST_Reader.h"
//
#define BATCH             100
#define LOOP_FOR_TRAIN    2350
// 10,000 is number of Test data
#define LOOP_FOR_TEST     (10000 / BATCH)

int main(int argc, char const *argv[]) {
    // create input, label data placeholder, placeholder is always managed by NeuralNetwork
    Operator<float> *x     = new Placeholder<float>(Tensor<float>::Constants(1, BATCH, 1, 1, 784, 1.0), "x");
    Operator<float> *label = new Placeholder<float>(Tensor<float>::Constants(1, BATCH, 1, 1, 10, 0.0), "label");
    Operator<float> *res   = new Reshape<float>(x, 1, BATCH, 1, 28, 28, "reshape");

    // ======================= layer 1=======================
    Tensorholder<float> *w1    = new Tensorholder<float>(Tensor<float>::Truncated_normal(1, 10, 1, 3, 3, 0.0, 0.1), "weight");
    Tensorholder<float> *b1    = new Tensorholder<float>(Tensor<float>::Constants(1, 1, 1, 1, 10, 0.1), "bias");
    Operator<float> *conv1 = new Convolution4D<float>(res, w1, 1, 1, 1, 1, "convolution1");
    Operator<float> *add1  = new Addconv<float>(conv1, b1, "addconv1");
    Operator<float> *act1  = new Relu<float>(add1, "relu1");
    Operator<float> *pool1 = new Maxpooling4D<float>(act1, 2, 2, 2, 2, "maxpool1");

    // ======================= layer 2=======================
    Tensorholder<float> *w2    = new Tensorholder<float>(Tensor<float>::Truncated_normal(1, 10, 10, 3, 3, 0.0, 0.1), "weight");
    Tensorholder<float> *b2    = new Tensorholder<float>(Tensor<float>::Constants(1, 1, 1, 1, 10, 0.1), "bias");
    Operator<float> *conv2 = new Convolution4D<float>(pool1, w2, 1, 1, 1, 1, "convolution1");
    Operator<float> *add2  = new Addconv<float>(conv2, b2, "addconv1");
    Operator<float> *act2  = new Relu<float>(add2, "relu2");
    Operator<float> *pool2 = new Maxpooling4D<float>(act2, 2, 2, 2, 2, "maxpool2");

    // ======================= layer 3=======================
    Operator<float> *flat = new Reshape<float>(pool2, 1, BATCH, 1, 1, 5 * 5 * 10, "flat");

    Tensorholder<float> *w_flat = new Tensorholder<float>(Tensor<float>::Truncated_normal(1, 1, 1, 5 * 5 * 10, 10, 0.0, 0.1), "w");
    Tensorholder<float> *b_flat = new Tensorholder<float>(Tensor<float>::Zeros(1, 1, 1, 1, 10), "b");

    Operator<float> *matmul = new MatMul<float>(flat, w_flat, "matmul");
    Operator<float> *add    = new Add<float>(matmul, b_flat, "add");

    // ======================= Error=======================
    Operator<float> *err = new SoftmaxCrossEntropy<float>(add, label, 0.000001, "SCE"); // 중요 조건일 가능성 있음

    // ======================= Optimizer=======================
    Optimizer<float> *optimizer = new GradientDescentOptimizer<float>(err, 0.01, MINIMIZE);

    optimizer->AddTrainableTensor(w1);
    optimizer->AddTrainableData(b1);
    //
    optimizer->AddTrainableData(w2);
    optimizer->AddTrainableData(b2);
    //
    optimizer->AddTrainableData(w_flat);
    optimizer->AddTrainableData(b_flat);
    //
    //// ======================= Train=======================
    MNISTDataSet<float> *dataset = CreateMNISTDataSet<float>();

    //
    for (int i = 0; i < LOOP_FOR_TRAIN; i++) {
        dataset->CreateTrainDataPair(BATCH);
        x->SetResult(dataset->GetTrainFeedImage());
        label->SetResult(dataset->GetTrainFeedLabel());

        // ======================= Forward=======================
        res->ComputeForwardPropagate();

        conv1->ComputeForwardPropagate();
        add1->ComputeForwardPropagate();
        act1->ComputeForwardPropagate();
        pool1->ComputeForwardPropagate();

        conv2->ComputeForwardPropagate();
        add2->ComputeForwardPropagate();
        act2->ComputeForwardPropagate();
        pool2->ComputeForwardPropagate();

        flat->ComputeForwardPropagate();
        matmul->ComputeForwardPropagate();
        add->ComputeForwardPropagate();
        err->ComputeForwardPropagate();

        std::cout << "Loop : " << i << " ";

        std::cout << "Train Accuracy is : "
        << (float)temp::Accuracy(add->GetResult(), label->GetResult(), BATCH)
        << '\n';

        //// ======================= Backward=======================
        err->ComputeBackPropagate();
        add->ComputeBackPropagate();
        matmul->ComputeBackPropagate();
        flat->ComputeBackPropagate();

        pool2->ComputeBackPropagate();
        act2->ComputeBackPropagate();
        add2->ComputeBackPropagate();
        conv2->ComputeBackPropagate();

        pool1->ComputeBackPropagate();
        act1->ComputeBackPropagate();
        add1->ComputeBackPropagate();
        conv1->ComputeBackPropagate();
        res->ComputeBackPropagate();

        b_flat->ComputeBackPropagate();
        w_flat->ComputeBackPropagate();
        b2->ComputeBackPropagate();
        w2->ComputeBackPropagate();
        b1->ComputeBackPropagate();
        w1->ComputeBackPropagate();
        //
        //// ======================= Update=======================
        optimizer->UpdateVariable();
        // std::cout << label->GetResult() << '\n';
        // std::cout << conv1->GetResult() << '\n';

        // HGUNN.Run(optimizer);

        // if ((i % 100) == 0) {
        //     // std::cout << err->GetResult()->GetShape() << '\n';
        //     // std::cout << add->GetResult()->GetShape() << '\n';
        //     // std::cout << matmul->GetResult()->GetShape() << '\n';
        //     // std::cout << flat->GetResult()->GetShape() << '\n';
        //     // std::cout << act2->GetResult()->GetShape() << '\n';
        //     // std::cout << add2->GetResult()->GetShape() << '\n';
        //     // std::cout << conv2->GetResult()->GetShape() << '\n';
        //     // std::cout << act1->GetResult()->GetShape() << '\n';
        //     // std::cout << add1->GetResult()->GetShape() << '\n';
        //     // std::cout << conv1->GetResult()->GetShape() << '\n';
        //     // std::cout << res->GetResult()->GetShape() << '\n';
        //     // std::cout << x->GetResult()->GetShape() << '\n';
        //
        //     std::cout << "Loop : " << i << " ";
        //
        //     std::cout << "Train Accuracy is : "
        //               << (float)temp::Accuracy(add->GetResult(), label->GetResult(), BATCH)
        //               << '\n';
        // }
    }

    // ======================= Testing =======================
    double test_accuracy = 0.0;

    for (int i = 0; i < (int)LOOP_FOR_TEST; i++) {
        dataset->CreateTestDataPair(BATCH);
        x->SetResult(dataset->GetTestFeedImage());
        label->SetResult(dataset->GetTestFeedLabel());

        // ======================= Forward=======================
        res->ComputeForwardPropagate();

        conv1->ComputeForwardPropagate();
        add1->ComputeForwardPropagate();
        act1->ComputeForwardPropagate();
        pool1->ComputeForwardPropagate();

        conv2->ComputeForwardPropagate();
        add2->ComputeForwardPropagate();
        act2->ComputeForwardPropagate();
        pool2->ComputeForwardPropagate();

        flat->ComputeForwardPropagate();
        matmul->ComputeForwardPropagate();
        add->ComputeForwardPropagate();
        err->ComputeForwardPropagate();

        // HGUNN.Run(err);
        // I'll implement flexibility about the situation that change of Batch size
        test_accuracy += (float)temp::Accuracy(add->GetResult(), label->GetResult(), BATCH);
    }

    std::cout << "Test Accuracy is : " << test_accuracy / (int)LOOP_FOR_TEST << '\n';


    return 0;
}
