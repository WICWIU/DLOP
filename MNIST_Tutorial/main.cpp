/*g++ -g -o testing -std=c++11 main.cpp ../Header/Shape.cpp ../Header/Data.cpp ../Header/Tensor.cpp ../Header/Operator.cpp ../Header/Objective_.cpp ../Header/Optimizer.cpp ../Header/NeuralNetwork_.cpp*/

#include <iostream>
#include <string>

#include "model//CNN.h"
#include "model//NN.h"

#include "..//Header//Temporary_method.h"
#include "MNIST_Reader.h"

#define BATCH             100
#define EPOCH             50
#define LOOP_FOR_TRAIN    (60000 / BATCH)
// 10,000 is number of Test data
#define LOOP_FOR_TEST     (10000 / BATCH)

int main(int argc, char const *argv[]) {
    // create input, label data placeholder
    Placeholder<float> *x = new Placeholder<float>(1, BATCH, 1, 1, 784, "x");
    Placeholder<float> *label = new Placeholder<float>(1, BATCH, 1, 1, 10, "label");

    // Result of classification
    Operator<float> *result = NULL;
    Tensor<float> *loss = NULL;

    // ======================= Select model ===================
    NeuralNetwork<float> *model = NULL;
    Objective<float> *objective = NULL;
    Optimizer<float> *optimizer = NULL;
    // model = new CNN(x, label, BATCH);
    // model = new NN(x, label);
    model = new CNN(x, BATCH);
    // model = new NN(x);
    objective = new SoftmaxCrossEntropy<float>(model, 0.0000001, "SCE");
    optimizer = model->GetOptimizer();

    // ======================= Prepare Data ===================
    MNISTDataSet<float> *dataset = CreateMNISTDataSet<float>();

    // pytorch check하기
    for (int i = 0; i < EPOCH; i++) {
        std::cout << "EPOCH : " << i << '\n';
        // ======================= Training =======================
        double train_accuracy = 0.f;

        for (int j = 0; j < LOOP_FOR_TRAIN; j++) {
            dataset->CreateTrainDataPair(BATCH);
            x->SetTensor(dataset->GetTrainFeedImage());
            label->SetTensor(dataset->GetTrainFeedLabel());

            result = model->ForwardPropagate();
            loss = objective->ForwardPropagate(label);
            objective->BackPropagate();
            model->BackPropagate();
            optimizer->UpdateVariable();

            train_accuracy += (float)temp::Accuracy(result->GetResult(), label->GetResult(), BATCH);
            printf("\rTraining complete percentage is %d / %d -> acc : %f", j + 1, LOOP_FOR_TRAIN, train_accuracy / (j + 1));
            fflush(stdout);
        }
        std::cout << '\n';

        // Caution!
        // Actually, we need to split training set between two set for training set and validation set
        // but in this example we do not above action.
        // ======================= Testing ======================
        double test_accuracy = 0.f;

        for (int j = 0; j < (int)LOOP_FOR_TEST; j++) {
            dataset->CreateTestDataPair(BATCH);
            x->SetTensor(dataset->GetTestFeedImage());
            label->SetTensor(dataset->GetTestFeedLabel());

            result = model->ForwardPropagate();

            test_accuracy += (float)temp::Accuracy(result->GetResult(), label->GetResult(), BATCH);
            printf("\rTesting complete percentage is %d / %d -> acc : %f", j + 1, LOOP_FOR_TEST, test_accuracy / (j + 1));
            fflush(stdout);
        }
        std::cout << '\n';
    }

    // we need to save best weight and bias when occur best acc on test time
    delete dataset;
    delete model;
    delete objective;

    return 0;
}
