#include "StockAnalyzer.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <numeric>

size_t StockAnalyzer::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

StockAnalyzer::StockAnalyzer() {
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    // Get initial access token
    accessToken = getAccessToken();

    sentimentLexicon = {
        {"bullish", 1.0},
        {"bearish", -1.0},
        {"buy", 1.0},
        {"sell", -1.0},
        {"moon", 1.0},
        {"dump", -1.0},
        {"long", 0.8},
        {"short", -0.8},
        {"calls", 0.7},
        {"puts", -0.7},
        // WSB specific terms
        {"tendies", 0.8},
        {"yolo", 0.6},
        {"diamond", 0.7},
        {"hands", 0.7},
        {"rocket", 0.8},
        {"squeeze", 0.6}
    };
}

StockAnalyzer::~StockAnalyzer() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
}

std::string StockAnalyzer::getAccessToken() {
    std::string auth_header = "Authorization: Basic " + clientId + ":" + clientSecret;
    std::string readBuffer;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, "User-Agent: StockAnalyzerBot/1.0");

    curl_easy_setopt(curl, CURLOPT_URL, "https://www.reddit.com/api/v1/access_token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "grant_type=client_credentials");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to get access token");
    }

    // Extract token from response
    size_t token_start = readBuffer.find("\"access_token\": \"");
    size_t token_end = readBuffer.find("\"", token_start + 16);
    return readBuffer.substr(token_start + 16, token_end - (token_start + 16));
}

std::string StockAnalyzer::makeRedditRequest(const std::string& subreddit, const std::string& query) {
    std::string url = "https://oauth.reddit.com/r/" + subreddit + "/search.json?q=" + query + "&sort=hot&limit=100";
    std::string readBuffer;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    headers = curl_slist_append(headers, ("User-Agent: " + userAgent).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to fetch Reddit data");
    }

    return readBuffer;
}

std::vector<StockAnalyzer::RedditPost> StockAnalyzer::getRedditPosts(const std::string& symbol) {
    std::vector<RedditPost> posts;
    std::vector<std::string> subreddits = {"wallstreetbets", "stocks", "investing", "stockmarket"};

    for (const auto& subreddit : subreddits) {
        try {
            std::string response = makeRedditRequest(subreddit, symbol);
            
            // Parse response and extract posts
            size_t pos = 0;
            while ((pos = response.find("\"title\":\"", pos)) != std::string::npos) {
                RedditPost post;
                
                // Extract title
                pos += 9;
                size_t end = response.find("\"", pos);
                post.title = response.substr(pos, end - pos);
                
                // Extract score
                pos = response.find("\"score\":", pos);
                if (pos != std::string::npos) {
                    pos += 8;
                    end = response.find(",", pos);
                    post.score = std::stoi(response.substr(pos, end - pos));
                }

                // Calculate sentiment
                post.sentiment = calculateSentiment(post.title);
                posts.push_back(post);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error fetching from r/" << subreddit << ": " << e.what() << "\n";
        }
    }

    return posts;
}

void StockAnalyzer::analyzeStock(const std::string& symbol) {
    std::cout << "\nAnalyzing " << symbol << " using live Reddit data...\n\n";

    // Start progress bar thread
    bool analysisComplete = false;
    std::thread progressThread([&]() {
        int progress = 0;
        while (!analysisComplete) {
            displayProgressBar(progress);
            progress = (progress + 1) % 100;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        displayProgressBar(100);
    });

    try {
        // Fetch and analyze Reddit posts
        auto posts = getRedditPosts(symbol);
        
        // Calculate weighted sentiment
        double totalSentiment = 0.0;
        double totalWeight = 0.0;
        
        for (const auto& post : posts) {
            double weight = log10(post.score + 10.0);  // Weight by post score
            totalSentiment += post.sentiment * weight;
            totalWeight += weight;
        }

        AnalysisResult result;
        result.sentimentScore = totalWeight > 0 ? totalSentiment / totalWeight : 0.0;
        result.confidenceScore = std::min(0.95, log10(posts.size() + 1) / 2.0);

        // Generate recommendation
        if (result.sentimentScore > 0.3) {
            result.recommendation = "BUY";
        } else if (result.sentimentScore < -0.3) {
            result.recommendation = "SELL";
        } else {
            result.recommendation = "HOLD";
        }

        analysisComplete = true;
        progressThread.join();

        std::cout << "\n\nAnalysis complete! Found " << posts.size() << " relevant posts.\n\n";
        std::cout << "=== Analysis Results ===\n"
                  << "Reddit Sentiment Score: " << std::fixed << std::setprecision(2) 
                  << result.sentimentScore << "\n"
                  << "Confidence Score: " << result.confidenceScore * 100 << "%\n"
                  << "Recommendation: " << result.recommendation << "\n\n";

        // Generate report
        generateReport(symbol, result);

    } catch (const std::exception& e) {
        analysisComplete = true;
        progressThread.join();
        std::cerr << "\nError during analysis: " << e.what() << "\n";
    }
}