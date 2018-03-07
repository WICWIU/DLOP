#include <iostream>
#include <string>

#include "..//..//Header//NeuralNetwork.h"

class my_CNN : public NeuralNetwork<float>{
private:
public:
    my_CNN(Placeholder<float> *x) {
        Operator<float> *out = NULL;

        // AddPlaceholder(x);
        // AddPlaceholder(label);

        int batch_size = x->GetResult()->GetBatchSize();

        out = AddOperator(new Reshape<float>(x, 1, batch_size, 1, 28, 28, "reshape"));

        // ======================= layer 1=======================
        out = AddConvLayer(out, 1, 10, "1");

        // ======================= layer 2=======================
        out = AddConvLayer(out, 10, 10, "2");
        out = AddOperator(new Reshape<float>(out, 1, batch_size, 1, 1, 5 * 5 * 10, "flat"));

        // ======================= layer 3=======================
        out = AddFullyConnectedLayer(out, 5 * 5 * 10, 250, "3");

        // ======================= layer 4=======================
        out = AddFullyConnectedLayer(out, 250, 10, "4");
    }

    Operator<float>* AddConvLayer(Operator<float> *pInput, int pChannelSize_in, int pChannelSize_out, std::string pLayernum) {
        Operator<float> *out = NULL;

        Operator<float> *weight = AddTensorholder(new Tensorholder<float>(Tensor<float>::Truncated_normal(1, pChannelSize_out, pChannelSize_in, 3, 3, 0.0, 0.1), "conv_weight" + pLayernum));
        Operator<float> *bias   = AddTensorholder(new Tensorholder<float>(Tensor<float>::Constants(1, 1, 1, 1, pChannelSize_out, 0.1), "conv_bias" + pLayernum));

        out = AddOperator(new Convolution2D<float>(pInput, weight, 1, 1, 1, 1, VALID, "conv" + pLayernum));
        out = AddOperator(new Addconv<float>(out, bias, "conv_add" + pLayernum));
        out = AddOperator(new Relu<float>(out, "conv_relu" + pLayernum));
        out = AddOperator(new Maxpooling2D<float>(out, 2, 2, 2, 2, VALID, "maxpool" + pLayernum));

        return out;
    }

    Operator<float>* AddFullyConnectedLayer(Operator<float> *pInput, int pColSize_in, int pColSize_out, std::string pLayernum) {
        Operator<float> *out = NULL;

        Operator<float> *weight = AddTensorholder(new Tensorholder<float>(Tensor<float>::Truncated_normal(1, 1, 1, pColSize_in, pColSize_out, 0.0, 0.1), "fc_weight" + pLayernum));
        Operator<float> *bias   = AddTensorholder(new Tensorholder<float>(Tensor<float>::Constants(1, 1, 1, 1, pColSize_out, 0.1), "fc_bias" + pLayernum));

        out = AddOperator(new MatMul<float>(pInput, weight, "fc_matmul" + pLayernum));
        out = AddOperator(new Add<float>(out, bias, "fc_add" + pLayernum));

        return out;
    }

    virtual ~my_CNN() {}
};