#ifndef STOCK_ANALYZER_HPP
#define STOCK_ANALYZER_HPP

#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include "/opt/homebrew/Cellar/jsoncpp/1.9.6/include/json/json.h"

class StockAnalyzer 
{
private:
    struct RedditPost 
    {
        std::string title;
        std::string body;
        int score;
        int num_comments;
        std::string created_utc;
        std::string author;
        std::string subreddit;
        std::string url;
        bool is_dd;
        double sentiment;
    };

    struct SubredditData 
    {
        std::string name;
        double weight;  
        int min_score;  
    };

    // Reddit credentials
    const std::string clientId = "bHCtkLr8jrZ_itWNMb8Olg";
    const std::string clientSecret = "gFTplLNr7bCRcsBXkPqCofJhssqpAA";
    const std::string userAgent = "StockAnalyzerBot/1.0";

    // Subreddits to analyze with their weights
    const std::vector<SubredditData> subreddits = 
    {
        {"wallstreetbets", 1.0, 50},    // High activity, meme-heavy
        {"stocks", 1.2, 20},            // More serious discussion
        {"investing", 1.2, 20},         // Traditional investing
        {"SecurityAnalysis", 1.3, 10},  // Deep analysis
        {"StockMarket", 1.1, 20},       // General market discussion
        {"options", 0.9, 20}            // Options trading focus
    };

    CURL* curl;
    std::string accessToken;
    std::map<std::string, double> sentimentLexicon;
    Json::Value cachedData;  // Cache for Reddit responses

    // Helper methods
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    std::string makeRedditRequest(const std::string& endpoint, const std::string& params);
    bool parseRedditPost(const Json::Value& postData, RedditPost& post);
    std::vector<RedditPost> getRedditPosts(const std::string& symbol);
    double calculateSentiment(const std::string& text);
    void generateReport(const std::string& symbol, const std::vector<RedditPost>& posts);
    std::string getAccessToken();
    void displayProgressBar(int progress);

public:
    StockAnalyzer();
    ~StockAnalyzer();
    void analyzeStock(const std::string& symbol);
};

#endif