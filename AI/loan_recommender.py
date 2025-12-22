import face_recognition
import numpy as np
from flask import Flask, request, jsonify
import joblib
import base64
import cv2
import json
from PIL import Image
import io

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
loan_rules = [
    # Best case scenarios
    (('income', 'High'), ('stability', 'VeryStable'), ('debt', 'Low'), ('recommendation', 'Large')),
    (('income', 'High'), ('stability', 'Stable'),     ('debt', 'Low'), ('recommendation', 'Large')),
    (('income', 'Medium'), ('stability', 'VeryStable'), ('debt', 'Low'), ('recommendation', 'Medium')),

    # Average scenarios
    (('income', 'Medium'), ('stability', 'Stable'), ('debt', 'Acceptable'), ('recommendation', 'Small')),
    (('income', 'High'), ('stability', 'Stable'), ('debt', 'Acceptable'), ('recommendation', 'Medium')),

    # Risky scenarios
    (('income', 'Low'), ('stability', 'Stable'), ('debt', 'Low'), ('recommendation', 'Small')),
    (('income', 'Medium'), ('stability', 'Unstable'), ('debt', 'Acceptable'), ('recommendation', 'Reject')),
    (('income', 'High'), ('stability', 'VeryStable'), ('debt', 'High'), ('recommendation', 'Small')),

    # Rejection scenarios
    (('stability', 'Unstable'), ('recommendation', 'Reject')),
    (('debt', 'High'), ('recommendation', 'Reject')),
    (('income', 'Low'), ('debt', 'Acceptable'), ('recommendation', 'Reject')),
]
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
    for rule in loan_rules:
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

# -------------------------------------------------------------------
# SECTION 5: FLASK SERVER & AI MODEL LOADING
# -------------------------------------------------------------------

app = Flask(__name__)

# --- Load the AI Chatbot Model (on server startup) ---
print("Loading AI chatbot model...")
try:
    chatbot_model = joblib.load('../../AI/bot_model.joblib')
    print("Chatbot model loaded successfully.")
except FileNotFoundError:
    print("ERROR: bot_model.joblib not found. Run train_bot.py first!")
    chatbot_model = None

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
def base64_to_image(base64_string):
    if not base64_string:
        return None
    try:
        # Remove data URL prefix if present
        if ',' in base64_string:
            print(f"Removing data URL prefix...")
            base64_string = base64_string.split(',')[1]

        # Decode base64
        img_bytes = base64.b64decode(base64_string)
        print(f"Decoded {len(img_bytes)} bytes")

        # Open with PIL
        pil_img = Image.open(io.BytesIO(img_bytes))
        print(f"PIL image mode: {pil_img.mode}, size: {pil_img.size}")

        # Convert to RGB (face_recognition needs RGB)
        if pil_img.mode != 'RGB':
            print(f"Converting from {pil_img.mode} to RGB")
            pil_img = pil_img.convert('RGB')

        # Convert to numpy array with explicit dtype
        img_array = np.array(pil_img, dtype=np.uint8)

        print(f"Final numpy array - shape: {img_array.shape}, dtype: {img_array.dtype}")

        return img_array

    except Exception as e:
        print(f"Error decoding image: {e}")
        import traceback
        traceback.print_exc()
        return None

@app.route('/get-face-vector', methods=['POST'])
def get_face_vector():
    data = request.get_json()
    image_b64 = data.get('image')

    if not image_b64:
        return jsonify({'success': False, 'message': 'No image data received'})

    # Decode image
    img = base64_to_image(image_b64)

    if img is None:
        return jsonify({'success': False, 'message': 'Image decoding failed'})

    # CRITICAL DEBUG INFO
    print(f"=== DEBUG INFO ===")
    print(f"Image shape: {img.shape}")
    print(f"Image dtype: {img.dtype}")
    print(f"Image type: {type(img)}")
    print(f"Is contiguous: {img.flags['C_CONTIGUOUS']}")
    print(f"Min/Max values: {img.min()}, {img.max()}")

    # Check if it's actually uint8 RGB
    if img.dtype != np.uint8:
        print(f"WARNING: dtype is {img.dtype}, converting to uint8")
        img = img.astype(np.uint8)

    if len(img.shape) != 3 or img.shape[2] != 3:
        print(f"ERROR: Image is not 3-channel RGB! Shape: {img.shape}")
        return jsonify({'success': False, 'message': f'Invalid image format: {img.shape}'})

    # Make sure it's contiguous
    if not img.flags['C_CONTIGUOUS']:
        print("WARNING: Array not contiguous, fixing...")
        img = np.ascontiguousarray(img)

    try:
        # Get face encodings
        encodings = face_recognition.face_encodings(img)

        if len(encodings) > 0:
            return jsonify({'success': True, 'vector': encodings[0].tolist()})
        else:
            return jsonify({'success': False, 'message': 'No face detected'})

    except Exception as e:
        print(f"Face recognition error: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({'success': False, 'message': f'Face processing error: {str(e)}'})

# -------------------------------------------------------------------
# SECTION 6: RUN THE SERVER
# -------------------------------------------------------------------

if __name__ == '__main__':
    print("Starting Flask server...")
    app.run(host='127.0.0.1', port=5000, debug=False)
