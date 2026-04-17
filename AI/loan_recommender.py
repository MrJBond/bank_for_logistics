from deepface import DeepFace
import sys
import io
import numpy as np
from flask import Flask, request, jsonify
import joblib
import base64
import cv2
import json
from PIL import Image
import tempfile
import os
from sklearn.ensemble import IsolationForest
from sklearn.feature_extraction.text import CountVectorizer
from sklearn.naive_bayes import MultinomialNB
from sklearn.pipeline import make_pipeline
from sklearn.linear_model import LinearRegression
import logging
import psycopg2

# Force stdout/stderr to use UTF-8 to prevent emoji crashes on Windows consoles
if sys.stdout.encoding != 'utf-8':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
if sys.stderr.encoding != 'utf-8':
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')


# -------------------------------------------------------------------
# SECTION 1: FUZZY LOGIC UTILITIES (Evaluator)
# -------------------------------------------------------------------
def trimf(x, a, b, c):
    """Triangular membership function."""
    x = np.asarray(x, dtype=float)
    y = np.zeros_like(x)
    if b > a:
        mask = (a < x) & (x <= b)
        y[mask] = (x[mask] - a) / (b - a)
    if c > b:
        mask = (b < x) & (x < c)
        y[mask] = (c - x[mask]) / (c - b)
    y[x == b] = 1.0
    return np.clip(y, 0.0, 1.0)

def l_shoulder(x, a, b):
    """Left-shoulder function. Stays at 1.0 for all x <= a."""
    x = np.asarray(x, dtype=float)
    y = np.zeros_like(x)
    # Flat
    y[x <= a] = 1.0
    # Falling
    if b > a:
        mask = (a < x) & (x < b)
        y[mask] = (b - x[mask]) / (b - a)
    return np.clip(y, 0.0, 1.0)

def r_shoulder(x, a, b):
    """Right-shoulder function. Stays at 1.0 for all x >= b."""
    x = np.asarray(x, dtype=float)
    y = np.zeros_like(x)
    # Rising
    if b > a:
        mask = (a < x) & (x <= b)
        y[mask] = (x[mask] - a) / (b - a)
    # Flat
    y[x >= b] = 1.0
    return np.clip(y, 0.0, 1.0)

def centroid_defuzz(y_grid, mu):
    """Centroid defuzzification method."""
    if np.sum(mu) == 0: return 0.0
    num = np.trapz(mu * y_grid, y_grid)
    denom = np.trapz(mu, y_grid)
    return 0.0 if denom == 0 else num / denom

# -------------------------------------------------------------------
# SECTION 2: FUZZY PROBLEM DEFINITION (Loan recommendation) (Evaluator)
# -------------------------------------------------------------------

# Input 1: Average Monthly Income ($)
income_min, income_max = 0, 10000
income_terms = {
    'Low':    lambda x: l_shoulder(x, 1500, 2500),
    'Medium': lambda x: trimf(x, 1500, 3500, 6000),
    'High':   lambda x: r_shoulder(x, 4000, 7000),
}

# Input 2: Financial Stability (Lower std dev is better)
stability_min, stability_max = 0, 5000
stability_terms = {
    'Unstable':   lambda x: r_shoulder(x, 1000, 2500),
    'Stable':     lambda x: trimf(x, 200, 600, 1500),
    'VeryStable': lambda x: l_shoulder(x, 200, 400),
}

# Input 3: Existing Debt ($)
debt_min, debt_max = 0, 100000
debt_terms = {
    'Low':        lambda x: l_shoulder(x, 5000, 10000),
    'Acceptable': lambda x: trimf(x, 5000, 20000, 40000),
    'High':       lambda x: r_shoulder(x, 30000, 50000),
}

# Output: Loan Recommendation Score (0-100)
score_min, score_max = 0, 100
score_terms = {
    'Reject': lambda y: trimf(y, score_min, score_min, 25),
    'Small':  lambda y: trimf(y, 10, 35, 60),
    'Medium': lambda y: trimf(y, 40, 65, 90),
    'Large':  lambda y: trimf(y, 75, score_max, score_max),
}
# -------------------------------------------------------------------
# SECTION 3: RULE BASE (The Loan Officer's Brain)
# -------------------------------------------------------------------
# -- 1. Create the fuzzy rules table
# CREATE TABLE IF NOT EXISTS loan_fuzzy_rules (
#     id SERIAL PRIMARY KEY,
#     income_level VARCHAR(50),     -- 'High', 'Medium', 'Low' or NULL
#     stability_level VARCHAR(50),  -- 'VeryStable', 'Stable', 'Unstable' or NULL
#     debt_level VARCHAR(50),       -- 'High', 'Acceptable', 'Low' or NULL
#     recommendation VARCHAR(50) NOT NULL
# );

# -- 2. Insert the rules
# INSERT INTO loan_fuzzy_rules (income_level, stability_level, debt_level, recommendation) VALUES
#     -- Best case scenarios
#     ('High', 'VeryStable', 'Low', 'Large'),
#     ('High', 'Stable', 'Low', 'Large'),
#     ('Medium', 'VeryStable', 'Low', 'Medium'),

#     -- Average scenarios
#     ('Medium', 'Stable', 'Acceptable', 'Small'),
#     ('High', 'Stable', 'Acceptable', 'Medium'),

#     -- Risky scenarios
#     ('Low', 'Stable', 'Low', 'Small'),
#     ('Medium', 'Unstable', 'Acceptable', 'Reject'),
#     ('High', 'VeryStable', 'High', 'Small'),

#     -- Rejection scenarios (the NULLs for conditions we don't care about)
#     (NULL, 'Unstable', NULL, 'Reject'),
#     (NULL, NULL, 'High', 'Reject'),
#     ('Low', NULL, 'Acceptable', 'Reject');
def load_fuzzy_rules_from_db():
    print("Fetching fuzzy loan rules from PostgreSQL...")
    loan_rules = []

    try:
        conn = psycopg2.connect(dbname="Bank", user="postgres", password="qwerty")
        cursor = conn.cursor()

        # Fetch all rules
        cursor.execute("SELECT income_level, stability_level, debt_level, recommendation FROM loan_fuzzy_rules")
        rows = cursor.fetchall()

        for row in rows:
            income, stability, debt, recommendation = row

            # Dynamically build the rule tuple, skipping NULLs
            current_rule = []
            if income:
                current_rule.append(('income', income))
            if stability:
                current_rule.append(('stability', stability))
            if debt:
                current_rule.append(('debt', debt))

            # The recommendation is always required
            current_rule.append(('recommendation', recommendation))

            # Convert list to tuple and add to our master list
            loan_rules.append(tuple(current_rule))

        cursor.close()
        conn.close()
        print(f"✅ Successfully loaded {len(loan_rules)} fuzzy rules from DB!")
        return loan_rules

    except Exception as e:
        print(f"⚠️ Failed to load rules from DB: {e}")
        return []


# -------------------------------------------------------------------
# SECTION 4: FUZZY INFERENCE ENGINE (Evaluator)
# -------------------------------------------------------------------

def get_loan_recommendation_score(input_data):
    # Fuzzify all inputs
    mu_income = {name: mf(input_data['avg_income']) for name, mf in income_terms.items()}
    mu_stability = {name: mf(input_data['stability']) for name, mf in stability_terms.items()}
    mu_debt = {name: mf(input_data['existing_debt']) for name, mf in debt_terms.items()}

    mu_inputs = {'income': mu_income, 'stability': mu_stability, 'debt': mu_debt}

    output_grid = np.linspace(score_min, score_max, 101)
    agg_mu = np.zeros_like(output_grid)

    # Evaluate rules
    for rule in global_loan_rules:
        antecedents = rule[:-1]
        consequent_term = rule[-1][1]

        strength = 1.0
        for var, term in antecedents:
            strength = min(strength, mu_inputs[var][term])

        if strength > 0:
            clipped_mf = np.minimum(score_terms[consequent_term](output_grid), strength)
            agg_mu = np.maximum(agg_mu, clipped_mf)

    # Defuzzify to get a crisp score
    return centroid_defuzz(output_grid, agg_mu)

# ---------------------------------------------------------
# SECTION 5: TRAIN THE TRANSACTION CATEGORIZE MODEL
# ---------------------------------------------------------
# MOVED TO train_transaction_categorizer.py


# -------------------------------------------------------------------
# SECTION 6: FLASK SERVER & AI MODEL LOADING
# -------------------------------------------------------------------

app = Flask(__name__)

global_loan_rules = load_fuzzy_rules_from_db()

# --- Load the AI Chatbot Model (on server startup) ---
print("Loading AI chatbot model...")
try:
    chatbot_model = joblib.load('../../AI/bot_model.joblib')
    print("Chatbot model loaded successfully.")
except FileNotFoundError:
    print("ERROR: bot_model.joblib not found. Run train_bot.py first!")
    chatbot_model = None

# --- Load the AI transaction categorizer Model (on server startup) ---
try:
    category_model = joblib.load('../../AI/transaction_model.joblib')
    print("✅ Pre-trained categorizer model loaded.")
except FileNotFoundError:
    print("⚠️ WARNING: transaction_model.joblib not found. Run train_model.py first!")
    category_model = None

# -------------------------------------------------------------------
# ENDPOINT 1: THE AI CHATBOT (Classifier)
# -------------------------------------------------------------------

@app.route('/chat', methods=['POST'])
def handle_chat():
    data = request.get_json()
    if not data or 'message' not in data:
        return jsonify({'error': 'No message provided'}), 400
    if chatbot_model is None:
        return jsonify({'reply': 'Error: Chatbot model is not loaded.'})

    user_message = data['message']

    # 1. Use the AI to PREDICT the intent
    predicted_intent = chatbot_model.predict([user_message])[0]

    # 2. Map the predicted intent to a response
    if predicted_intent == 'check_balance':
        response = {'intent': 'check_balance', 'reply': 'Getting your balance...'}
    elif predicted_intent == 'request_loan':
        response = {'intent': 'request_loan_recommendation', 'reply': 'I can help with that. Let me analyze your account...'}
    elif predicted_intent == 'list_transactions':
        response = {'intent': 'list_transactions', 'reply': 'Here are your recent transactions:'}
    elif predicted_intent == 'greeting':
        response = {'intent': 'greeting', 'reply': 'Hello! How can I help?'}
    elif predicted_intent == 'assess_risk':
        # The reply text here is just a placeholder.
        # C++ app will immediately take over and show the first question.
        response = {
            'intent': 'assess_risk',
            'reply': 'Sure, let me help you determine your investment risk profile.'
        }
    elif predicted_intent == 'gratitude':
        response = {
            'intent': 'gratitude',
            'reply': "You're welcome! Let me know if you need help with anything else."
        }
    else:
        response = {'intent': 'unknown', 'reply': "Sorry, I'm not sure."}

    return jsonify(response)

# -------------------------------------------------------------------
# ENDPOINT 2: THE FUZZY LOAN RECOMMENDER (Evaluator)
# -------------------------------------------------------------------

@app.route('/recommend-loan', methods=['POST'])
def recommend_loan():
    data = request.get_json()
    required_fields = ['avg_income', 'stability', 'existing_debt']
    if not all(field in data for field in required_fields):
        return jsonify({'error': 'Missing required fields'}), 400

    score = get_loan_recommendation_score(data)

    return jsonify({
        'inputs_received': data,
        'recommendation_score': round(score, 2)
    })

#--------------------------------------------------------------------
# ENDPOINT 3: PROCESS THE FACE VECTOR
#--------------------------------------------------------------------
@app.route('/get-face-vector', methods=['POST'])
def get_face_vector():
    print("!!! ROUTE HIT - get_face_vector (DeepFace) !!!", flush=True)

    try:
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'message': 'No data'})

        b64_str = data.get('image')
        if not b64_str:
            return jsonify({'success': False, 'message': 'No image data'})

        # ---------------------------------------------------------
        # 1. DECODE IMAGE (Standard OpenCV)
        # ---------------------------------------------------------
        if "," in b64_str:
            b64_str = b64_str.split(",")[1]

        img_bytes = base64.b64decode(b64_str)
        nparr = np.frombuffer(img_bytes, np.uint8)
        img_bgr = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

        if img_bgr is None:
            return jsonify({'success': False, 'message': 'Image decoding failed'})

        print(f"Debug: Image decoded. Shape: {img_bgr.shape}", flush=True)

        # ---------------------------------------------------------
        # 2. GENERATE VECTOR (Using DeepFace)
        # ---------------------------------------------------------
        print("Debug: Calling DeepFace...", flush=True)

        # 'Facenet' model creates a 128-dimensional vector (similar to dlib).
        # 'opencv' detector is fast and reliable.
        # This function handles all preprocessing internally.
        embedding_objs = DeepFace.represent(
            img_path = img_bgr,
            model_name = "Facenet",
            detector_backend = "opencv",
            enforce_detection = True
        )

        # DeepFace returns a list of results (one for each face found).
        # We take the first one.
        vector = embedding_objs[0]["embedding"]

        print(f"Success! Vector generated. Length: {len(vector)}", flush=True)

        return jsonify({'success': True, 'vector': vector})

    except ValueError as ve:
        # DeepFace throws ValueError if enforce_detection=True and no face is found
        # We convert exception to string safely to avoid emoji crashes
        safe_msg = str(ve).encode('ascii', 'ignore').decode('ascii')
        print(f"Face Detection Info: {safe_msg}", flush=True)
        return jsonify({'success': False, 'message': 'No face detected'})

    except Exception as e:
        # Safe string conversion for generic errors too
        safe_msg = str(e).encode('ascii', 'ignore').decode('ascii')
        print(f"CRITICAL SERVER ERROR: {safe_msg}", flush=True)
        import traceback
        traceback.print_exc()
        return jsonify({'success': False, 'message': f'Server error: {safe_msg}'}), 500

# -------------------------------------------------------------------
# ENDPOINT 4: FRAUD DETECTION (Anomaly Detection)
# -------------------------------------------------------------------
@app.route('/check-fraud', methods=['POST'])
def check_fraud():
    print('!!!!! ROUTE HIT - check-fraud !!!!!', flush = True)
    try:
        data = request.get_json()
        new_amount = data.get('amount')
        history = data.get('history') # List of float amounts

        print(f"Debug: Amount={new_amount}, HistoryLen={len(history) if history else 0}", flush=True)

        if new_amount is None:
            return jsonify({'error': 'Missing amount'}), 400

        # 1. Edge Case: Not enough data to judge
        # If user is new (fewer than 5 transactions), we can't determine anomalies.
        if len(history) < 5:
            return jsonify({'status': 'SAFE', 'reason': 'Not enough history'})

        # 2. Prepare Data for Scikit-Learn
        # IsolationForest expects a 2D array: [[10], [20], [15]...]
        X_history = np.array(history).reshape(-1, 1)
        X_new = np.array([[new_amount]])

        # 3. Train Isolation Forest
        # contamination='auto' lets the model decide threshold,
        # or use 0.1 (assume 10% of txns might be weird)
        clf = IsolationForest(contamination=0.1, random_state=42)
        clf.fit(X_history)

        # 4. Predict
        # Returns -1 for Outlier (Fraud), 1 for Inlier (Safe)
        prediction = clf.predict(X_new)

        # Get anomaly score (lower is more abnormal)
        score = clf.decision_function(X_new)[0]

        print(f"Fraud Check: Amount={new_amount}, Pred={prediction[0]}, Score={score:.2f}", flush=True)

        if prediction[0] == -1:
            return jsonify({'status': 'SUSPICIOUS', 'score': score})
        else:
            return jsonify({'status': 'SAFE', 'score': score})

    except Exception as e:
        print(f"FRAUD CHECK ERROR: {e}", flush=True)
        return jsonify({'error': str(e)}), 500

# ---------------------------------------------------------
# ENDPOINT 5: TRANSACTION CATEGORIZE
# ---------------------------------------------------------
@app.route('/categorize', methods=['POST'])
def categorize_transaction():
    try:
        if category_model is None:
            return jsonify({'error': 'Model not initialized'}), 500

        data = request.get_json()
        description = data.get('description', '')

        if not description:
            return jsonify({'category': 'Unknown', 'icon': '❓'})

        # 1. Get Probabilities
        probs = category_model.predict_proba([description])[0]
        classes = category_model.classes_

        max_prob = np.max(probs)
        max_index = np.argmax(probs)
        predicted_cat = classes[max_index]

        # 2. Confidence Check (lowered slightly to 0.35 due to char-grams being more "fuzzy")
        if max_prob < 0.35:
            final_cat = "Unknown"
        else:
            final_cat = predicted_cat

        icons = {
            "Food": "🍔", "Transport": "🚖", "Bills": "💡", "Shopping": "🛒",
            "Transfer": "💸", "Cash": "🏧", "Income": "💰", "Unknown": "❔",

            "Fuel": "⛽",         # For gas stations, diesel, charging networks
            "Maintenance": "🔧",  # For auto repairs, oil changes, spare parts
            "Tolls": "🛣️",        # For highway tolls, weigh stations, and parking
            "Lodging": "🏨",      # For driver hotels and overnight stays during long routes
            "Insurance": "🛡️",    # For cargo insurance, vehicle policies
            "Customs": "🛂"       # For border fees, tariffs, or port charges
        }
        icon = icons.get(final_cat, "💳")

        return jsonify({'category': final_cat, 'icon': icon, 'confidence': float(max_prob)})

    except Exception as e:
        print(f"Categorization Error: {e}", flush=True)
        return jsonify({'category': 'Error', 'icon': '⚠️'})


# ---------------------------------------------------------
# ENDPOINT 6: BATCH CATEGORIZE
# ---------------------------------------------------------
@app.route('/categorize-list', methods=['POST'])
def categorize_transaction_list():
    try:
        if category_model is None:
            return jsonify({'error': 'Model not initialized'}), 500
        data = request.get_json()
        descriptions = data.get('descriptions', [])

        if not descriptions:
            return jsonify({'results': []})

        probs_list = category_model.predict_proba(descriptions)
        classes = category_model.classes_

        icons = {
            "Food": "🍔", "Transport": "🚖", "Bills": "💡", "Shopping": "🛒",
            "Transfer": "💸", "Cash": "🏧", "Income": "💰", "Unknown": "❔",

            "Fuel": "⛽",         # For gas stations, diesel, charging networks
            "Maintenance": "🔧",  # For auto repairs, oil changes, spare parts
            "Tolls": "🛣️",        # For highway tolls, weigh stations, and parking
            "Lodging": "🏨",      # For driver hotels and overnight stays during long routes
            "Insurance": "🛡️",    # For cargo insurance, vehicle policies
            "Customs": "🛂"       # For border fees, tariffs, or port charges
        }

        results = []

        for i, probs in enumerate(probs_list):
            max_prob = np.max(probs)
            max_index = np.argmax(probs)
            predicted_cat = classes[max_index]

            if max_prob < 0.35:
                final_cat = "Unknown"
            else:
                final_cat = predicted_cat

            icon = icons.get(final_cat, "💳")

            results.append({
                'category': final_cat,
                'icon': icon,
                'confidence': float(max_prob)
            })

        return jsonify({'results': results})

    except Exception as e:
        print(f"Batch Categorization Error: {e}", flush=True)
        return jsonify({'error': str(e)}), 500

# -------------------------------------------------------------------
# ENDPOINT 7: SPENDING FORECAST
# -------------------------------------------------------------------
@app.route('/forecast-spending', methods=['POST'])
def forecast_spending():
    try:
        data = request.get_json()
        # Expecting a list of monthly totals: [500.0, 450.50, 600.0, 550.0, ...]
        history = data.get('history', [])

        if not history or len(history) < 2:
            return jsonify({'success': False, 'message': 'Not enough data (need at least 2 months)'})

        # 1. Prepare Data
        # X = Month Index [0, 1, 2, 3...]
        # y = Spending Amount [500, 450, 600...]
        X = np.array(range(len(history))).reshape(-1, 1)
        y = np.array(history)

        # 2. Train Model
        model = LinearRegression()
        model.fit(X, y)

        # 3. Predict Next Month
        next_month_index = len(history)
        prediction = model.predict([[next_month_index]])[0]

        # 4. Calculate Trend (Slope)
        # Positive slope = Spending is increasing
        # Negative slope = Spending is decreasing
        trend = model.coef_[0]

        return jsonify({
            'success': True,
            'prediction': float(prediction),
            'trend': float(trend)
        })
    except Exception as e:
        print(f"Forecast Error: {e}", flush=True)
        return jsonify({'success': False, 'error': str(e)}), 500

# -------------------------------------------------------------------
# SECTION 7: RUN THE SERVER
# -------------------------------------------------------------------

if __name__ == '__main__':
    # Force Flask and Werkzeug to write directly to standard error
    logging.getLogger('werkzeug').disabled = False
    app.logger.addHandler(logging.StreamHandler(sys.stderr))
    app.logger.setLevel(logging.DEBUG)
    print("\n=== Final registered routes ===")
    for rule in app.url_map.iter_rules():
        print(f"{rule.endpoint}: {rule.rule} [{', '.join(rule.methods)}]")
    print("================================\n")
    print("Starting Flask server...", flush=True)
    app.run(host='127.0.0.1', port=5000, debug=False, threaded=True)
