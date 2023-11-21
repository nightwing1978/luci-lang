#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>

int main(int argc, char** argv)
{

    std::vector<std::shared_ptr<int>> v;

    v.push_back(std::make_shared<int>(2));
    v.push_back(std::make_shared<int>(0));
    v.push_back(std::make_shared<int>(1));

    std::cerr << "v=" << *v[0] << "," << *v[1] << "," << *v[2] << std::endl;

    try
    {
        std::vector< int > ordering;
        ordering.resize(v.size());
        for (size_t i=0; i < v.size();++i)
            ordering[i] = i;

        std::sort(ordering.begin(), ordering.end(), [v](const int& a, const int& b) -> bool {
            return *v[a] < *v[b];
        });

        std::vector<std::shared_ptr<int>> b;
        b.resize(v.size());
        for (size_t i=0; i < ordering.size();++i)
            b[i] = std::move(v[ordering[i]]);

        v = std::move(b);
    }
    catch(const std::exception& /*e*/)
    {
    }
    
    std::cerr << "v=" << *v[0] << "," << *v[1] << "," << *v[2] << std::endl;
    return 0;
}


// int main(int argc, char** argv)
// {

//     std::vector<std::shared_ptr<int>> v;

//     v.push_back(std::make_shared<int>(2));
//     v.push_back(std::make_shared<int>(1));
//     v.push_back(std::make_shared<int>(0));

//     std::cerr << "v=" << v[0] << "," << v[1] << "," << v[2] << std::endl;

//     try
//     {
//         std::vector< int > ordering;
//         ordering.resize(v.size());
//         for (int i=0;i<v.size();++i)
//             ordering[i] = i;

//         std::sort(ordering.begin(), ordering.end(), [v](const int& a, const int& b) -> bool {
//             throw std::runtime_error("");
//         });


//         std::sort(v.begin(), v.end(), [](const std::shared_ptr<int>& /*a*/, const std::shared_ptr<int>& /*b*/) -> bool {
//             throw std::runtime_error("");
//         });
//     }
//     catch(const std::exception& /*e*/)
//     {
//     }
    
//     std::cerr << "v=" << v[0] << "," << v[1] << "," << v[2] << std::endl;
//     return 0;
// }