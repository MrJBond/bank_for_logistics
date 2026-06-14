# 🚚 AI-Driven Financial Management System for Logistics

An intelligent client-server FinTech platform designed specifically for transportation and logistics centers. This system bridges the "information gap" between traditional banking and logistics telematics by integrating computer vision, natural language processing (NLP), spatial geofencing, and neuro-fuzzy models.

---

## 🌟 Key Features

* **Biometric Step-Up Authentication (Face ID):** Captures video via OpenCV on the C++ client and verifies the driver's identity using DeepFace algorithms on the Python backend (calculating Euclidean distance) when anomalous transactions are detected.
* **Geofencing & Route Verification:** Integrates OpenRouteService (HGV profile) for route planning and uses the `Shapely` library to create a spatial "security corridor." Transactions occurring outside this polygon trigger fraud alerts.
* **Smart Expense Categorization (NLP):** Replaces inaccurate MCC codes by analyzing raw, unstructured receipt text. Uses TF-IDF with character n-grams and a Multinomial Naive Bayes classifier to automatically categorize expenses (e.g., Fuel, Maintenance, Tolls).
* **Dynamic Anti-Fraud System:** Employs an Isolation Forest unsupervised machine learning model to detect spending anomalies based on the individual historical behavior of each driver.
* **Fuzzy Logic Credit Scoring:** Replaces rigid banking limits with a Mamdani fuzzy inference system (utilizing Centroid defuzzification) to evaluate a driver's creditworthiness for emergency fuel loans.
* **Expense Forecasting:** Uses Linear Regression to analyze historical data and predict future corporate budget trends.
* **Intelligent Chatbot:** Provides a command-line-like interface using a Support Vector Classifier (SVC) for highly accurate intent classification, allowing users to navigate the system using natural language.

---

## 🛠 Tech Stack

### Frontend (C++ Client)

* **C++ 17 & Qt 6:** High-performance, cross-platform desktop application with a High Contrast Dark Theme optimized for driving conditions.
* **Qt Modules:** `QtWidgets` (UI), `QNetworkAccessManager` (Asynchronous REST API calls), `QWebEngine` (Embedded Chromium for maps).
* **OpenCV:** Low-level webcam capture and frame conversion (BGR to RGB).
* **Leaflet.js:** Interactive GeoJSON route rendering.

### Backend (Python AI Server)

* **Python 3.10+ & Flask:** Lightweight RESTful API architecture.
* **Scikit-learn:** ML pipelines (TF-IDF Vectorizer, MultinomialNB, SVC, Isolation Forest, Linear Regression).
* **DeepFace:** VGG-Face / Facenet architectures for generating 128/2622-dimensional face embeddings.
* **Shapely:** Computational geometry for Point-in-Polygon spatial analysis.
* **Scikit-Fuzzy:** Fuzzy logic rule evaluation and defuzzification.

### Database

* **PostgreSQL:** ACID-compliant relational storage.
* **JSONB:** Utilized for efficient storage of complex spatial route geometries without requiring heavy spatial extensions.

---

## 🏗 Architecture (3-Tier)

1. **Presentation Tier (Client):** Implements the Repository and Singleton design patterns. Handles local data validation, video capture, and asynchronous HTTP communication via Signals and Slots to prevent UI blocking.
2. **Application Tier (Server):** The "AI Core." Processes JSON payloads, communicates with external APIs (OpenRouteService), runs machine learning inference, and returns actionable status codes and metrics.
3. **Data Tier (Database):** Stores user profiles, transaction history, isolated face embeddings (for privacy), and dynamically loaded Fuzzy Logic rules and chatbot training data.

---

## 🚀 Getting Started

### Prerequisites

* C++ Compiler (GCC, Clang, or MSVC)
* Qt 6.x (with WebEngine and Multimedia modules)
* Python 3.10+
* PostgreSQL 14+
* OpenCV C++ libraries installed and linked

### Backend Setup

1. Clone the repository and navigate to the `backend` directory.
2. Create a virtual environment and install dependencies:
```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt

```



```
3. Configure your PostgreSQL database and add the connection string.
4. Run the Flask development server:
   ```bash
python app.py

```

### Frontend Setup

1. Open the project file in Qt Creator.
2. Ensure the OpenCV include paths and libraries are correctly configured in your build system (`.pro` or `CMakeLists.txt`).
3. Build and run the C++ application.

---

## 📊 Machine Learning Metrics

* **NLP Categorization (MultinomialNB):** Achieves an overarching F1-Score of 0.82 on highly unstructured logistics data (resilient to typos and abbreviations).
* **Chatbot Intent (SVC):** Demonstrates 78.3% accuracy under stress-testing with slang, typos, and out-of-domain queries.
* **Continuous Learning:** Borderline inferences (confidence threshold < 35%) are flagged as `Unknown` for manual review by managers, subsequently updating the PostgreSQL training dataset for continuous model improvement.