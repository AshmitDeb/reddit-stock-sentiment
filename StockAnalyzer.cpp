#include "StockAnalyzer.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <cmath>
#include "/opt/homebrew/Cellar/jsoncpp/1.9.6/include/json/json.h"

size_t StockAnalyzer::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) 
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

StockAnalyzer::StockAnalyzer() : curl(nullptr) 
{
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    // Initialize sentiment lexicon
    sentimentLexicon = 
    {
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
        {"tendies", 0.8},
        {"rocket", 0.8},
        {"yolo", 0.5}
    };

    accessToken = getAccessToken();
}

StockAnalyzer::~StockAnalyzer() 
{
    if (curl) 
    {
        curl_easy_cleanup(curl);
    }
}

bool StockAnalyzer::parseRedditPost(const Json::Value& postData, RedditPost& post) 
{
    try 
    {
        post.title = postData["title"].asString();
        post.body = postData["selftext"].asString();
        post.score = postData["score"].asInt();
        post.num_comments = postData["num_comments"].asInt();
        post.created_utc = postData["created_utc"].asString();
        post.author = postData["author"].asString();
        post.subreddit = postData["subreddit"].asString();
        post.url = postData["url"].asString();
        
        // Check if it's a DD post
        std::string title_lower = post.title;
        std::transform(title_lower.begin(), title_lower.end(), title_lower.begin(), ::tolower);
        post.is_dd = (title_lower.find("dd") != std::string::npos) || 
                     (title_lower.find("due diligence") != std::string::npos);

        // Calculate initial sentiment
        post.sentiment = calculateSentiment(post.title + " " + post.body);
        
        return true;
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error parsing post: " << e.what() << "\n";
        return false;
    }
}

void StockAnalyzer::displayProgressBar(int progress) 
{
    std::cout << "\r[";
    for (int i = 0; i < 50; i++) 
    {
        if (i < progress / 2) 
        {
            std::cout << "=";
        } 
        else 
        {
            std::cout << " ";
        }
    }
    std::cout << "] " << progress << "%" << std::flush;
}

double StockAnalyzer::calculateSentiment(const std::string& text) 
{
    double sentiment = 0.0;
    size_t count = 0;
    
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    for (const auto& [word, value] : sentimentLexicon) 
    {
        if (lowerText.find(word) != std::string::npos)
        {
            sentiment += value;
            count++;
        }
    }
    
    return count > 0 ? sentiment / static_cast<double>(count) : 0.0;
}

std::string StockAnalyzer::getAccessToken() 
{
    std::string readBuffer;
    struct curl_slist* headers = NULL;
    
    // Create the user agent header
    std::string userAgentHeader = "User-Agent: " + userAgent;
    headers = curl_slist_append(headers, userAgentHeader.c_str());
    
    // Set up basic auth and other headers
    curl_easy_setopt(curl, CURLOPT_USERNAME, clientId.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, clientSecret.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.reddit.com/api/v1/access_token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "grant_type=client_credentials");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Enable verbose debug output
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) 
    {
        std::cerr << "Failed to get access token: " << curl_easy_strerror(res) << std::endl;
        std::cerr << "Response: " << readBuffer << std::endl;
        throw std::runtime_error("Failed to get access token");
    }
    
    std::cout << "Token Response: " << readBuffer << std::endl;
    
    // Parse JSON response
    Json::Value root;
    Json::Reader reader;
    
    if (reader.parse(readBuffer, root)) 
    {
        if (root.isMember("access_token")) 
        {
            return root["access_token"].asString();
        }
        std::cerr << "No access token in response" << std::endl;
    }
    
    std::cerr << "Failed to parse JSON response" << std::endl;
    return "";
}
std::string StockAnalyzer::makeRedditRequest(const std::string& endpoint, const std::string& params) 
{
    std::string url = "https://oauth.reddit.com" + endpoint;
    if (!params.empty()) 
    {
        url += "?" + params;
    }
    std::string readBuffer;

    std::cout << "\nMaking request to URL: " << url << std::endl;
    
    struct curl_slist* headers = NULL;
    // Use the exact format that worked in the curl command
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    headers = curl_slist_append(headers, "User-Agent: StockAnalyzerBot/1.0");
    
    curl_easy_reset(curl);  // Reset all options
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    
    std::cout << "Using access token: " << accessToken << std::endl;
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) 
    {
        std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
        throw std::runtime_error("Failed to make Reddit request");
    }

    std::cout << "API Response: " << readBuffer << std::endl;
    return readBuffer;
}

std::vector<StockAnalyzer::RedditPost> StockAnalyzer::getRedditPosts(const std::string& symbol) 
{
    std::vector<RedditPost> allPosts;
    char* encodedSymbol = curl_easy_escape(curl, symbol.c_str(), static_cast<int>(symbol.length()));
    
    std::cout << "\nSearching for symbol: " << symbol << std::endl;
    
    for (const auto& subreddit : subreddits) 
    {
        try {
            // Modified search parameters to use Reddit's search syntax
            std::string searchTerm = std::string(encodedSymbol);
            std::string params = "q=" + searchTerm + 
                               "&restrict_sr=1" +  // Search only this subreddit
                               "&sort=hot" +
                               "&t=week" +
                               "&limit=100" +
                               "&type=link";  // Only get posts, not comments
            
            std::cout << "\nSearching in r/" << subreddit.name << std::endl;
            std::cout << "Search parameters: " << params << std::endl;
            
            std::string response = makeRedditRequest("/r/" + subreddit.name + "/search.json", params);
            
            // If we get a valid response, parse it
            if (!response.empty()) 
            {
                Json::Value root;
                Json::Reader reader;
                
                if (reader.parse(response, root)) 
                {
                    if (root.isMember("data") && root["data"].isMember("children")) 
                    {
                        const Json::Value& posts = root["data"]["children"];
                        for (const auto& postWrapper : posts) 
                        {
                            if (postWrapper.isMember("data")) 
                            {
                                RedditPost post;
                                if (parseRedditPost(postWrapper["data"], post)) 
                                {
                                    if (post.score >= subreddit.min_score) 
                                    {
                                        post.sentiment *= subreddit.weight;
                                        allPosts.push_back(post);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            std::cout << "Processed r/" << subreddit.name << ": Found " 
                     << allPosts.size() << " relevant posts\n";
            
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Error processing r/" << subreddit.name << ": " 
                     << e.what() << "\n";
            // Continue with next subreddit even if this one fails
            continue;
        }
    }
    
    curl_free(encodedSymbol);
    
    // Sort posts by score
    std::sort(allPosts.begin(), allPosts.end(), 
        [](const RedditPost& a, const RedditPost& b) 
        {
            return a.score > b.score;
        });
    
    return allPosts;
}

void StockAnalyzer::generateReport(const std::string& symbol, 
                                 const std::vector<RedditPost>& posts) 
                                 {
    if (!std::filesystem::exists("reports")) 
    {
        std::filesystem::create_directory("reports");
    }
    
    std::ofstream report("reports/" + symbol + "_analysis.html");
    report << "<!DOCTYPE html><html><head>\n"
           << "<title>Reddit Stock Analysis: " << symbol << "</title>\n"
           << "<style>\n"
           << "body { font-family: Arial, sans-serif; margin: 40px; }\n"
           << ".metric { margin: 20px 0; padding: 15px; background: #f5f5f5; "
           << "border-radius: 5px; }\n"
           << ".post { margin: 10px 0; padding: 10px; border: 1px solid #ddd; }\n"
           << ".positive { color: green; }\n"
           << ".negative { color: red; }\n"
           << "</style></head><body>\n"
           << "<h1>Stock Analysis Report: " << symbol << "</h1>\n";

    // Add top posts section
    report << "<div class='metric'>\n"
           << "<h2>Top Reddit Posts</h2>\n";
    
    int postCount = 0;
    for (const auto& post : posts) 
    {
        if (postCount++ >= 10) break;  // Show top 10 posts
        
        report << "<div class='post'>\n"
               << "<h3>" << post.title << "</h3>\n"
               << "<p>Score: " << post.score << " | Comments: " 
               << post.num_comments << "</p>\n"
               << "<p>Subreddit: r/" << post.subreddit << "</p>\n"
               << "<p>Sentiment: " << std::fixed << std::setprecision(2) 
               << post.sentiment << "</p>\n"
               << "</div>\n";
    }
    
    report << "</div></body></html>";
    report.close();
}

void StockAnalyzer::analyzeStock(const std::string& symbol) 
{
    std::cout << "\nAnalyzing " << symbol << " across " << subreddits.size() 
              << " subreddits...\n\n";

    bool analysisComplete = false;
    std::thread progressThread([this, &analysisComplete]() 
    {
        int progress = 0;
        while (!analysisComplete) {
            displayProgressBar(progress);
            progress = (progress + 2) % 100;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        displayProgressBar(100);
    });

    try {
        auto posts = getRedditPosts(symbol);
        
        // Calculate metrics
        double totalSentiment = 0.0;
        double totalWeight = 0.0;
        std::map<std::string, int> subredditCounts;
        int ddPosts = 0;
        
        for (const auto& post : posts) 
        {
            double weight = std::log10(post.score + post.num_comments + 10.0);
            if (post.is_dd) 
            {
                weight *= 1.5;
                ddPosts++;
            }
            
            totalSentiment += post.sentiment * weight;
            totalWeight += weight;
            subredditCounts[post.subreddit]++;
        }

        double avgSentiment = totalWeight > 0 ? totalSentiment / totalWeight : 0.0;
        double confidence = std::min(0.95, std::log10(posts.size() + 1) / 2.0);

        std::cout << "\n\nAnalysis complete!\n\n";
        std::cout << "=== " << symbol << " Analysis Results ===\n"
                 << "Total Posts: " << posts.size() << "\n"
                 << "DD Posts: " << ddPosts << "\n"
                 << "Sentiment: " << std::fixed << std::setprecision(2) 
                 << avgSentiment << "\n"
                 << "Confidence: " << confidence * 100 << "%\n\n";

        std::cout << "Subreddit Breakdown:\n";
        for (const auto& [subreddit, count] : subredditCounts) 
        {
            std::cout << "r/" << subreddit << ": " << count << " posts\n";
        }

        // Generate detailed recommendation
        std::string recommendation;
        std::string reasoning;

        if (posts.size() < 5) 
        {
            recommendation = "INSUFFICIENT DATA";
            reasoning = "Not enough Reddit discussion to make a confident recommendation.";
        } 
        else 
        {
            if (avgSentiment > 0.5 && confidence > 0.7) 
            {
                recommendation = "STRONG BUY";
                reasoning = "High positive sentiment with strong confidence level.";
            } 
            else if (avgSentiment > 0.3) 
            {
                recommendation = "BUY";
                reasoning = "Moderate positive sentiment detected.";
            } 
            else if (avgSentiment < -0.5 && confidence > 0.7) 
            {
                recommendation = "STRONG SELL";
                reasoning = "High negative sentiment with strong confidence level.";
            } 
            else if (avgSentiment < -0.3) 
            {
                recommendation = "SELL";
                reasoning = "Moderate negative sentiment detected.";
            }
            else 
            {
                recommendation = "HOLD";
                reasoning = "Neutral or mixed sentiment detected.";
            }

            if (ddPosts > 5) 
            {
                reasoning += " Multiple due diligence posts found.";
            }
        }

        std::cout << "\nRECOMMENDATION: " << recommendation << "\n";
        std::cout << "REASONING: " << reasoning << "\n\n";

        // Generate detailed HTML report
        generateReport(symbol, posts);
        std::cout << "Detailed report saved to: reports/" << symbol << "_analysis.html\n";

    } 
    catch (const std::exception& e) 
    {
        std::cerr << "\nError during analysis: " << e.what() << "\n";
    }

    analysisComplete = true;
    progressThread.join();
}