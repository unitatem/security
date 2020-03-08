#include <iostream>
#include <string_view>

constexpr std::string_view text_table[] = {"PublicData", "PrivateData"};

int main()
{
    std::string_view text = text_table[0];
    std::cout << "valid length = " << text.size() << "\n";

    int length = text.size() + text_table[1].size() + 1;
    // This check is bad:
    // (unsigned long - int) => (unsigned long - unsigned long) => never less than 0
    if (text.size() - length < 0) {
        std::cout << "error that should be triggered\n";
        return -1;
    }
    std::cout << "length to print = " << length << "\n";
    std::cout << std::string_view(text.begin(), length) << "\n";
}