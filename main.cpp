#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include "StockAnalyzer.hpp"

void displayHelp() 
{
    std::cout << "\n=== Stock Sentiment Analyzer Pro ===\n"
              << "Commands:\n"
              << "  analyze <symbol>     - Analyze stock sentiment (e.g., 'analyze TSLA')\n"
              << "  help                 - Display this help\n"
              << "  quit                 - Exit program\n";
}

std::string toUpper(std::string str) 
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

int main() 
{
    std::cout << "=== Stock Sentiment Analyzer Pro ===\n\n";
    
    // Initialize analyzer - credentials are now handled internally
    StockAnalyzer analyzer;
    
    std::cout << "Reddit API configured. Ready to analyze stocks.\n";
    std::cout << "Type 'help' for commands or directly enter a stock symbol (e.g., 'TSLA')\n";

    std::string input;
    
    while (true) 
    {
        std::cout << "\nEnter command or stock symbol: ";
        std::getline(std::cin, input);
        
        // Remove leading/trailing whitespace
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);
        
        if (input.empty()) 
        {
            continue;
        }

        std::string upperInput = toUpper(input);
        
        if (upperInput == "QUIT") 
        {
            break;
        } 
        else if (upperInput == "HELP") 
        {
            displayHelp();
        } 
        else if (upperInput.substr(0, 8) == "ANALYZE ") 
        {
            std::string symbol = upperInput.substr(8);
            if (symbol.empty()) 
            {
                std::cout << "Please provide a stock symbol (e.g., analyze TSLA)\n";
                continue;
            }
            try 
            {
                analyzer.analyzeStock(symbol);
            } 
            catch (const std::exception& e) 
            {
                std::cerr << "Error: " << e.what() << "\n";
            }
        } 
        else 
        {
            // If input is not a command, treat it as a stock symbol
            std::string symbol = upperInput;
            if (symbol.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos) 
            {
                std::cout << "Invalid stock symbol. Please enter a valid symbol (e.g., TSLA)\n";
                continue;
            }
            try 
            {
                analyzer.analyzeStock(symbol);
            } 
            catch (const std::exception& e) 
            {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }
    }
    
    return 0;
}
