# Reddit Stock Sentiment Analyzer (C++17)

Multithreaded CLI tool that mines r/wallstreetbets, r/stocks and r/investing, scores each post with a finance-weighted sentiment model, and produces:

* **Live terminal output** with progress bar and summary stats  
* **HTML report** in `./reports/⟨TICKER⟩_analysis.html`

Core tech: C++17 threads, libcurl REST calls, JsonCpp parsing, RAII / STL, and weighted NLP. :contentReference[oaicite:0]{index=0}

---

## Features
* OAuth 2 flow to fetch a bearer token and query Reddit’s Search API :contentReference[oaicite:1]{index=1}  
* Weighted lexicon (`bullish`, `puts`, `rocket`, …) for sentiment scoring  
* Per-subreddit weight and score thresholds  
* Concurrency: one worker per subreddit plus a progress-bar thread :contentReference[oaicite:2]{index=2}  
* Auto-generated HTML report with top posts and summary metrics

---

## Prerequisites
| Library | Package name on Ubuntu / Homebrew |
|---------|-----------------------------------|
| **libcurl** | `libcurl4-openssl-dev` / `curl` |
| **JsonCpp** | `libjsoncpp-dev` / `jsoncpp` |
| C++17 compiler | `g++ >= 10` or `clang >= 12` |

> **Reddit credentials**  
> Create an **app** at <https://www.reddit.com/prefs/apps>.  
> Note the *Client ID* and *Client Secret* and replace the placeholders in `StockAnalyzer.hpp`.

---

## Build

### Quick one-liner

```bash
g++ -std=c++17 -O2 -pthread \
    src/*.cpp \
    -lcurl -ljsoncpp \
    -o sentiment_cli
