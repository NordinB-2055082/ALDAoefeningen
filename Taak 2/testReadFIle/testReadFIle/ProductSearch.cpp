#include "ProductSearch.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

ProductSearch::ProductSearch() : idTrieRoot(new TrieNode()), titleTrieRoot(new TrieNode()) {}

ProductSearch::~ProductSearch() {
    delete idTrieRoot;
    delete titleTrieRoot;
}

/*
Reading a product and putting it into the vector is seen as O(1)
*/
void ProductSearch::loadProducts(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (getline(file, line)) {
        // Split the line into fields
        std::vector < std::string > fields = split(line, ',');

        // Extract values for non-title fields
        std::string asin = fields[0];
        std::string title;
        // For some reason when there are double quotes in the line the asin starts with ", i can't find where it comes from, 
        // so i avoided this problem with the following simple code
        if (asin[0] == '"') {
            asin.erase(0, 1);
        }
        // Check if the title field is enclosed in double quotes
        if (fields[1][0] == '"') {
            // Concatenate all fields from the title field until the last field enclosed in double quotes
            for (size_t i = 1; i < fields.size(); ++i) {
                title += fields[i];
                if (fields[i].back() == '"') {
                    // If the last field ends with double quotes, remove them and break the loop
                    // If we don't do this we have "" in our data
                    title = removeQuotes(title);
                    break;
                }
                else {
                    title += ','; // Add a comma between fields
                }
            }
        }
        else {
            // The title field does not contain commas
            title = fields[1];
        }

        // Process the rest of the fields, 
        // we count from the back to the front to ignore when title made more fields because of the ',' in the value of title itself
        std::string imgUrl = fields[fields.size() - 9];
        std::string productURL = fields[fields.size() - 8];
        double stars = stod(fields[fields.size() - 7]);
        int reviews = stoi(fields[fields.size() - 6]);
        double price = stod(fields[fields.size() - 5]);
        double listPrice = stod(fields[fields.size() - 4]);
        int category_id = stoi(fields[fields.size() - 3]);
        bool isBestSeller = (fields[fields.size() - 2] == "True");
        int boughtInLastMonth = stoi(fields[fields.size() - 1]);
        int discount = 0;

        if (listPrice != 0.0) {
            double difference = listPrice - price;
            double differenceOnTotal = difference/listPrice;
            double discountPercentageFloat = differenceOnTotal * 100;
            discount = static_cast<int>(discountPercentageFloat);
        }
        
        Product product = { asin, title, imgUrl, productURL, stars, reviews, price, listPrice, category_id, isBestSeller, boughtInLastMonth, discount };

        //std::cout << "Title:" << product.title << ", Price: $" << product.price << "discount: " << product.discount << std::endl;
        products.push_back(product);
       
        insertProductToTrie(product, products.size() - 1, ID);
        insertProductToTrie(product, products.size() - 1, TITLE);

    }
    
    file.close();
}
vector<std::string> ProductSearch::split(const std::string& s, char delimiter) {
    vector<std::string> tokens;
    stringstream ss(s);
    std::string token;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

string ProductSearch::removeQuotes(const std::string& s) {
    std::string result = s;
    result.erase(remove(result.begin(), result.end(), '"'), result.end());
    return result;
}

void ProductSearch::insertProductToTrie(const Product& product, int index, TrieType type) {
    std::string key;

    if (type == ID) {
        key = product.asin + END_CHAR;
    }
    else if (type == TITLE) {
        key = product.title + END_CHAR;
    }

    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    TrieNode* node = (type == ID) ? idTrieRoot : titleTrieRoot;

    for (char c : key) {
        if (!node->children[c]) {
            node->children[c] = new TrieNode();
        }
        node = node->children[c];
        node->productIndices.push_back(index);
    }
}



std::vector<int> ProductSearch::searchProducts(const std::string& query, TrieType type) {
    std::string key = query;
    //key = query;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    TrieNode* node = (type == ID) ? idTrieRoot : titleTrieRoot;

    for (char c : key) {
        if (!node->children[c]) {
            return {};
        }
        node = node->children[c];
    }

    return node->productIndices;
}
void ProductSearch::printProducts(const std::vector<int>& indices, int start, int end) {
    for (int i = start; i <= end && i < indices.size(); ++i) {
        int index = indices[i];
        std::cout << "(" << i + 1 << ") " << products[index].title << " (" << products[index].discount << "%)" << std::endl;
    }
}


void ProductSearch::searchAndPrint(const std::string& query) {
    // Search by ID
    std::vector<int> idIndices = searchProducts(query, ID);

    // Search by title
    std::vector<int> titleIndices = searchProducts(query, TITLE);

    // Merge the indices from both searches
    std::vector<int> mergedIndices;
    std::merge(idIndices.begin(), idIndices.end(), titleIndices.begin(), titleIndices.end(), std::back_inserter(mergedIndices));
    // Sort and remove duplicates
    std::sort(mergedIndices.begin(), mergedIndices.end());
    mergedIndices.erase(std::unique(mergedIndices.begin(), mergedIndices.end()), mergedIndices.end());

    if (mergedIndices.empty()) {
        std::cout << "No matching products found." << std::endl;
        return;
    }

    std::sort(mergedIndices.begin(), mergedIndices.end(), [&](int a, int b) {
        return products[a].discount > products[b].discount;
        });

    int pageSize = 20;
    int start = 0;
    int end = std::min(static_cast<int>(mergedIndices.size()) - 1, pageSize - 1);

    while (true) {
        printProducts(mergedIndices, start, end);

        std::cout << "### Press a number to make your choice or press N or P for next/previous page." << std::endl;
        std::string input;
        std::getline(std::cin, input);

        if (input == "q") {
            break;
        }
        else if (input == "N" && end < mergedIndices.size() - 1) {
            start += pageSize;
            end = std::min(end + pageSize, static_cast<int>(mergedIndices.size()) - 1);
        }
        else if (input == "P" && start > 0) {
            start -= pageSize;
            end = std::min(end - pageSize, static_cast<int>(mergedIndices.size()) - 1);
        }
        else if (input != "") {
            int choice = std::stoi(input);
            if (choice > 0 && choice <= mergedIndices.size()) {
                int index = mergedIndices[start + choice - 1];
                std::cout << products[index].title << std::endl;
            }
        }
    }
}