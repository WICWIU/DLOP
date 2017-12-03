#ifndef META_PARAMETER_H_
#define META_PARAMETER_H_    value

// #include <iostream>
// #include <string>

#include "Tensor.h"

// 전방 선언
template<typename DTYPE> class Operator;

enum PADDING {
    ZERO,
    SAME
};

template<typename DTYPE>
class MetaParameter {
public:
    // 정의
    MetaParameter() {}

    virtual ~MetaParameter() {}
};

template<typename DTYPE>
class ConvParam : public MetaParameter<DTYPE>{
private:
    Operator<DTYPE> *filter = NULL;
    int *stride             = NULL;
    PADDING value;
    // std::string m_name     = NULL;
    void *data_format      = NULL;
    void *use_cudnn_on_gpu = NULL;

public:
    ConvParam(Operator<DTYPE> *pfilter, int pstrides0, int pstrides1, int pstrides2, int pstrides3, PADDING pvalue = SAME) {
        std::cout << "ConvParam::ConvParam()" << '\n';
        filter = pfilter;
        // stride 값들을 stride 값으로 옮기는 방안 생각
        value = pvalue;
    }

    virtual ~ConvParam() {}
};

template<typename DTYPE>
class MaxpoolParam : public MetaParameter<DTYPE>{
private:
    Tensor<DTYPE> *ksize = NULL;
    int *stride          = NULL;
    PADDING value;
    // std::string m_name     = NULL;
    void *data_format      = NULL;
    void *use_cudnn_on_gpu = NULL;

public:
    MaxpoolParam(int pksize0, int pksize1, int pksize2, int pksize3, int pstrides0, int pstrides1, int pstrides2, int pstrides3, PADDING pvalue = SAME) {
        std::cout << "MaxpoolParam::MaxpoolParam()" << '\n';
        // ksize 값들을 이용해서 마스크를 1로 채운 Tensor 생성
        // stride 값들을 stride 값으로 옮기는 방안 생각
        value = pvalue;
    }

    virtual ~MaxpoolParam() {}
};

#endif  // META_PARAMETER_H_
