#ifndef STOCK_ANALYZER_HPP
#define STOCK_ANALYZER_HPP

#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>

class StockAnalyzer {
private:
    struct RedditPost {
        std::string title;
        std::string body;
        int score;
        int num_comments;
        std::string created_utc;
        double sentiment;
    };

    struct AnalysisResult {
        double sentimentScore;
        double priceTarget;
        double confidenceScore;
        std::string recommendation;
        std::map<std::string, double> keyFactors;
        std::map<std::string, double> riskMetrics;
        std::vector<std::string> topComments;
    };

    // Reddit API credentials
    const std::string clientId = "bHCtkLr8jrZ_itWNMb8Olg";
    const std::string clientSecret = "gFTplLNr7bCRcsBXkPqCofJhssqpAA";
    const std::string userAgent = "StockAnalyzerBot/1.0";
    
    std::map<std::string, double> sentimentLexicon;
    CURL* curl;
    std::string accessToken;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    std::string makeRedditRequest(const std::string& subreddit, const std::string& query);
    std::string getAccessToken();
    std::vector<RedditPost> getRedditPosts(const std::string& symbol);
    double calculateSentiment(const std::string& text);
    void displayProgressBar(int progress);
    void generateReport(const std::string& symbol, const AnalysisResult& result);

public:
    StockAnalyzer(); 
    ~StockAnalyzer();
    void analyzeStock(const std::string& symbol);
};

#endif