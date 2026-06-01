"""
Run this file one time from the terminal (python train_transaction_categorizer.py).
This creates the transaction_model.joblib file, which is the AI's "brain."
"""

import psycopg2
import joblib
from sklearn.feature_extraction.text import CountVectorizer, TfidfVectorizer
from sklearn.naive_bayes import MultinomialNB
from sklearn.pipeline import make_pipeline

def retrain_categorizer():
    print("Connecting to PostgreSQL...")
    conn = psycopg2.connect(dbname="Bank", user="postgres", password="qwerty")
    cursor = conn.cursor()

    print("Fetching training data...")
    # table: transaction_rules (description TEXT, category TEXT)
    cursor.execute("SELECT description, category FROM transaction_rules")
    data = cursor.fetchall()

    if not data:
        print("No data found. Aborting training.")
        return

    X_train_text = [row[0] for row in data]
    y_train_labels = [row[1] for row in data]

    print("Training model...")
    # Create a Pipeline:
    # 1. Vectorizer: Converts text to a TF-IDF frequency matrix
    # 2. Classifier: Naive Bayes
    # analyzer='char_wb': Look at characters inside word boundaries
    # ngram_range=(2, 4): Look at patterns of 2 to 4 letters (e.g., "Ne", "etfl", "lix")
    category_model = make_pipeline(TfidfVectorizer(analyzer='char_wb', ngram_range=(2, 4)), MultinomialNB(alpha=0.1))
    category_model.fit(X_train_text, y_train_labels)

    print("Saving model to disk...")
    # Save the fully trained pipeline to a file
    joblib.dump(category_model, 'transaction_model.joblib')

    cursor.close()
    conn.close()
    print("✅ Model trained and saved successfully!")

if __name__ == "__main__":
    retrain_categorizer()

# e.g. data:
    # train_data = [
    #     # Transport
    #     ("Uber request", "Transport"), ("Uber", "Transport"), ("Lyft ride", "Transport"),
    #     ("Shell Station", "Transport"), ("Shell", "Transport"), ("Gas", "Transport"), ("Gas Station", "Transport"),
    #     ("Bus Ticket", "Transport"), ("Train", "Transport"),

    #     # Food
    #     ("McDonalds", "Food"), ("Mcdonalds", "Food"), ("Starbucks", "Food"),
    #     ("Burger King", "Food"), ("Whole Foods", "Food"), ("Dinner", "Food"), ("Lunch", "Food"),

    #     # Bills
    #     ("Netflix Subscription", "Bills"), ("Netflix", "Bills"), ("Spotify", "Bills"),
    #     ("Electric Bill", "Bills"), ("Rent payment", "Bills"), ("Water Bill", "Bills"), ("Phone Bill", "Bills"),

    #     # Shopping
    #     ("Walmart", "Shopping"), ("Amazon", "Shopping"), ("Target", "Shopping"), ("Nike Store", "Shopping"),
    #     ("Clothes", "Shopping"),

    #     # Transfers
    #     ("Money Transfer", "Transfer"), ("Sent to Account", "Transfer"),
    #     ("Wire Transfer", "Transfer"), ("Payment to", "Transfer"), ("Transfer", "Transfer"),

    #     # Cash/ATM
    #     ("ATM Withdrawal", "Cash"), ("Cash out", "Cash"), ("ATM", "Cash"),

    #     # Income/Salary
    #     ("Salary Deposit", "Income"), ("Paycheck", "Income"), ("Income", "Income")
    # ]
# SQL data:
    # --. Bulk insert the training data
    # INSERT INTO transaction_rules (description, category) VALUES
    #     -- Transport
    #     ('Uber request', 'Transport'),
    #     ('Uber', 'Transport'),
    #     ('Lyft ride', 'Transport'),
    #     ('Shell Station', 'Transport'),
    #     ('Shell', 'Transport'),
    #     ('Gas', 'Transport'),
    #     ('Gas Station', 'Transport'),
    #     ('Bus Ticket', 'Transport'),
    #     ('Train', 'Transport'),

    #     -- Food
    #     ('McDonalds', 'Food'),
    #     ('Mcdonalds', 'Food'),
    #     ('Starbucks', 'Food'),
    #     ('Burger King', 'Food'),
    #     ('Whole Foods', 'Food'),
    #     ('Dinner', 'Food'),
    #     ('Lunch', 'Food'),

    #     -- Bills
    #     ('Netflix Subscription', 'Bills'),
    #     ('Netflix', 'Bills'),
    #     ('Spotify', 'Bills'),
    #     ('Electric Bill', 'Bills'),
    #     ('Rent payment', 'Bills'),
    #     ('Water Bill', 'Bills'),
    #     ('Phone Bill', 'Bills'),

    #     -- Shopping
    #     ('Walmart', 'Shopping'),
    #     ('Amazon', 'Shopping'),
    #     ('Target', 'Shopping'),
    #     ('Nike Store', 'Shopping'),
    #     ('Clothes', 'Shopping'),

    #     -- Transfers
    #     ('Money Transfer', 'Transfer'),
    #     ('Sent to Account', 'Transfer'),
    #     ('Wire Transfer', 'Transfer'),
    #     ('Payment to', 'Transfer'),
    #     ('Transfer', 'Transfer'),

    #     -- Cash/ATM
    #     ('ATM Withdrawal', 'Cash'),
    #     ('Cash out', 'Cash'),
    #     ('ATM', 'Cash'),

    #     -- Income/Salary
    #     ('Salary Deposit', 'Income'),
    #     ('Paycheck', 'Income'),
    #     ('Income', 'Income');
