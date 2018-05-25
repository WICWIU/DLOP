#ifndef __SHAPE__
#define __SHAPE__    value

#include "Common.h"

class Shape {
private:
    int m_Rank;
    int *m_aDim;
    Device m_Device;

#if __CUDNN__
    cudnnTensorDescriptor_t m_desc;
#endif  // if __CUDNN__

private:
    int  Alloc(int pRank, ...);
    int  Alloc(Shape *pShape);
    void Delete();

#if __CUDNN__
    int  AllocOnGPU();
    void DeleteOnGPU();
    int  ReShapeOnGPU();
#endif  // if __CUDNN__

public:
    Shape(int pSize0, int pSize1, int pSize2, int pSize3, int pSize4);
    Shape(int pSize0, int pSize1, int pSize2, int pSize3);
    Shape(int pSize0, int pSize1, int pSize2);
    Shape(int pSize0, int pSize1);
    Shape(int pSize0);
    Shape(Shape *pShape);  // Copy Constructor
    virtual ~Shape();

    int  GetRank();
    int  GetDim(int pRanknum);
    int& operator[](int pRanknum);  // operator[] overload

    int  ReShape(int pSize0, int pSize1, int pSize2, int pSize3, int pSize4);
    int  ReShape(int pRank, ...);


#if __CUDNN__
    cudnnTensorDescriptor_t& GetDescriptor();
#endif  // __CUDNN__
};

std::ostream& operator<<(std::ostream& pOS, Shape *pShape);

#endif  // __SHAPE__
