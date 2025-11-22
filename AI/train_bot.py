
"""
Run this file one time from the terminal (python train_bot.py).
This creates the bot_model.joblib file, which is the AI's "brain."
"""

import joblib
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.svm import SVC
from sklearn.pipeline import make_pipeline

print("Training chatbot model...")

# 1. Our Training Data
training_data = [
    ("How much is in my account?", 'check_balance'),
    ("what's my balance", 'check_balance'),
    ("show me the money", 'check_balance'),


    ("How much of a loan can I get?", 'request_loan'),
    ("recommend a loan", 'request_loan'),
    ("am i eligible for a loan", 'request_loan'),


    ("show my transaction history", 'list_transactions'),
    ("what are my recent transactions", 'list_transactions'),
    ("list my payments", 'list_transactions'),


    ("hi", 'greeting'),
    ("hello", 'greeting'),


    ('unknown', 'unknown'),
    ('sdfsdgfsg', 'unknown'),
    ('i want a pizza', 'unknown'),
    ('what is the weather', 'unknown'),
    ('asdasd asdasd', 'unknown'),
    ('my car is blue', 'unknown'),


    ("assess my financial risk", 'assess_risk'),
    ("start risk quiz", 'assess_risk'),
    ("risk profile", 'assess_risk'),
    ("i want to answer some questions", 'assess_risk'),


    ("thanks", 'gratitude'),
    ("thank you", 'gratitude'),
    ("thanks a lot", 'gratitude'),
    ("thank you very much", 'gratitude'),
    ("thx", 'gratitude'),
    ("appreciate it", 'gratitude'),
    ("thanks for the help", 'gratitude'),
    ("cool thanks", 'gratitude'),
]

# 2. Separate data
X_train = [data[0] for data in training_data]
y_train = [data[1] for data in training_data]

# 3. Create the AI Pipeline
model_pipeline = make_pipeline(TfidfVectorizer(), SVC(kernel='linear'))

# 4. Train the Model
model_pipeline.fit(X_train, y_train)

# 5. Save the Model to a File
joblib.dump(model_pipeline, 'bot_model.joblib')

print("Model trained and saved as 'bot_model.joblib'!")
