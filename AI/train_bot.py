
"""
Run this file one time from the terminal (python train_bot.py).
This creates the bot_model.joblib file, which is the AI's "brain."
"""

import psycopg2
import joblib
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.svm import SVC
from sklearn.pipeline import make_pipeline

def train_chatbot():
    print("Connecting to PostgreSQL...")

    # 1. Connect to the database
    try:
        conn = psycopg2.connect(dbname="Bank", user="postgres", password=os.environ.get("DB_PASSWORD"))
        cursor = conn.cursor()
    except Exception as e:
        print(f"Database connection failed: {e}")
        return

    print("Fetching chatbot training data...")

    # 2. Fetch the data
    cursor.execute("SELECT phrase, intent FROM chatbot_training_data")
    data = cursor.fetchall()

    if not data:
        print("No training data found in the database. Aborting.")
        return

    # 3. Separate data into X (phrases) and y (intents)
    X_train = [row[0] for row in data]
    y_train = [row[1] for row in data]

    print(f"Loaded {len(X_train)} phrases across {len(set(y_train))} unique intents.")
    print("Training chatbot model...")

    # 4. Create and Train the AI Pipeline
    model_pipeline = make_pipeline(TfidfVectorizer(), SVC(kernel='linear'))
    model_pipeline.fit(X_train, y_train)

    # 5. Save the Model to a File
    joblib.dump(model_pipeline, 'bot_model.joblib')

    cursor.close()
    conn.close()
    print("✅ Model trained and saved as 'bot_model.joblib'!")

if __name__ == "__main__":
    train_chatbot()

# sql data:
    # -- 1. Create the table for chatbot training data
    # CREATE TABLE IF NOT EXISTS chatbot_training_data (
    #     id SERIAL PRIMARY KEY,
    #     phrase VARCHAR(255) NOT NULL,
    #     intent VARCHAR(100) NOT NULL
    # );

    # -- 2. Insert your initial dataset
    # INSERT INTO chatbot_training_data (phrase, intent) VALUES
    #     -- Balance
    #     ('How much is in my account?', 'check_balance'),
    #     ('what''s my balance', 'check_balance'),
    #     ('show me the money', 'check_balance'),

    #     -- Loans
    #     ('How much of a loan can I get?', 'request_loan'),
    #     ('recommend a loan', 'request_loan'),
    #     ('am i eligible for a loan', 'request_loan'),

    #     -- Transactions
    #     ('show my transaction history', 'list_transactions'),
    #     ('what are my recent transactions', 'list_transactions'),
    #     ('list my payments', 'list_transactions'),

    #     -- Greetings
    #     ('hi', 'greeting'),
    #     ('hello', 'greeting'),

    #     -- Risk Assessment
    #     ('assess my financial risk', 'assess_risk'),
    #     ('start risk quiz', 'assess_risk'),
    #     ('risk profile', 'assess_risk'),
    #     ('i want to answer some questions', 'assess_risk'),

    #     -- Gratitude
    #     ('thanks', 'gratitude'),
    #     ('thank you', 'gratitude'),
    #     ('thanks a lot', 'gratitude'),
    #     ('thank you very much', 'gratitude'),
    #     ('thx', 'gratitude'),
    #     ('appreciate it', 'gratitude'),
    #     ('thanks for the help', 'gratitude'),
    #     ('cool thanks', 'gratitude'),

    #     -- Unknown / Noise
    #     ('unknown', 'unknown'),
    #     ('sdfsdgfsg', 'unknown'),
    #     ('i want a pizza', 'unknown'),
    #     ('what is the weather', 'unknown'),
    #     ('asdasd asdasd', 'unknown'),
    #     ('my car is blue', 'unknown');
