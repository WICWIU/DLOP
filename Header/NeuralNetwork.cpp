#include "NeuralNetwork.h"

NeuralNetwork::NeuralNetwork() {
    std::cout << "NeuralNetwork::NeuralNetwork()" << '\n';
    Alloc();
}

NeuralNetwork::~NeuralNetwork() {
    Delete();
    std::cout << "NeuralNetwork::~NeuralNetwork()" << '\n';
}

// ===========================================================================================

bool NeuralNetwork::Alloc() {
    std::cout << "NeuralNetwork::Alloc()" << '\n';
    m_aEnd->AddEdgebetweenOperators(m_pStart);
    return true;
}

void NeuralNetwork::Delete() {
    std::cout << "NeuralNetwork::Delete()" << '\n';
    DeleteOperator();
    delete m_aEnd;
    delete m_aOptimizer;

}

// ===========================================================================================


bool NeuralNetwork::AllocOptimizer(Optimizer * pOptimizer) {
    pOptimizer->GetObjectOperator()->AllocOptimizer(pOptimizer);

    // Object Operator는 거의 100% Optimizer가 필요 없다.
    // m_aEnd->SetOptimizer(pOptimizer);
    return true;
}

bool NeuralNetwork::DeleteOperator() {
    m_aEnd->DeleteInputOperator();
    return true;
}

// ===========================================================================================

Operator * NeuralNetwork::AddPlaceholder(Tensor *pTensor, std::string pName) {
    std::cout << "NeuralNetwork::Placeholder(Tensor *, std::string )" << '\n';

    // placeholder의 경우 trainable하지 않다.
    Operator *temp = new Placeholder(pTensor, pName);

    temp->AddEdgebetweenOperators(m_pStart);

    return temp;
}

// ===========================================================================================

// 주소에 따라 조절되는 알고리즘 추가 필요
// forwardPropagate Algorithm 수정 필요
bool NeuralNetwork::ForwardPropagate(Operator *pStart, Operator *pEnd) {
    if (pEnd == NULL) {
        if (m_aEnd == NULL) {
            std::cout << "There is no linked Operator!" << '\n';
            return false;
        } else pEnd = m_aEnd;
    }

    pEnd->ForwardPropagate();

    return true;
}

bool NeuralNetwork::BackPropagate(Operator *pStart, Operator *pEnd) {
    if (pEnd == NULL) {
        if (m_aEnd == NULL) {
            std::cout << "There is no linked Operator!" << '\n';
            return false;
        } else pEnd = m_aOptimizer->GetObjectOperator();
    }

    // ObjectOperator로부터 시작한다
    pEnd->BackPropagate();

    return true;
}

// ===========================================================================================

bool NeuralNetwork::Training(Operator *pStart, Operator *pEnd) {
    std::cout << "\n<<<ForwardPropagate>>>\n" << '\n';

    ForwardPropagate(pStart, pEnd);

    std::cout << "\n<<<BackPropagate>>>\n" << '\n';

    BackPropagate(pStart, pEnd);

    std::cout << '\n';

    return true;
}

bool NeuralNetwork::Testing(Operator *pStart, Operator *pEnd) {
    std::cout << "\n<<<ForwardPropagate>>>\n" << '\n';

    ForwardPropagate(pStart, pEnd);

    std::cout << '\n';

    return true;
}

// ===========================================================================================

bool NeuralNetwork::CreateGraph(Optimizer* pOptimizer){

    // Final Operator

    // SetEndOperator();

    // =====================================

    SetOptimizer(pOptimizer);
    AllocOptimizer(pOptimizer);

    // =====================================

    return true;
}

// bool NeuralNetwork::CreateGraph(){
//     // 추후에 만들 Optimizer는 그 자체가 Trainable Operator주소를 가질 수 있도록 만들 것이다.
//     // factory method 삭제예정
// }
