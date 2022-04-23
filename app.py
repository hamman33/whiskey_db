from flask import Flask, render_template, url_for, request, redirect
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///whiskey.db'
db = SQLAlchemy(app)

class Ratings(db.Model):
    whiskey = db.Column(db.String(200), primary_key=True)
    rating = db.Column(db.Integer, nullable=False)
    date_drank = db.Column(db.DateTime, default=datetime.utcnow)

    def __repr__(self):
        return '<whiskey %r>' % self.whiskey

class Collection(db.Model):
    bottle = db.Column(db.String(200), primary_key=True)
    avg_rating = db.Column(db.Float, default=0)
    date_added = db.Column(db.DateTime, default=datetime.utcnow)
    date_killed = db.Column(db.DateTime)

    def __repr__(self):
        return '<bottle %r>' % self.bottle

@app.route('/', methods=['POST', 'GET'])
def index():
    if request.method == 'POST':
        if(len(request.form) == 1):
            print("here4")
            bottle = request.form['bottle']
            new_bottle = Collection(bottle=bottle)
            try:
                db.session.add(new_bottle)
                db.session.commit()
                return redirect('/')
            except:
                return 'There was an issue adding your task'
        else:
            
            print("here")
            whiskey = request.form['whiskey']
            rating = request.form['rating']
            new_rating = Ratings(whiskey=whiskey, rating=rating)

            try:
                db.session.add(new_rating)
                db.session.commit()
                return redirect('/')
            except:
                return 'There was an issue adding your task'

    else:
        ratings = Ratings.query.order_by(Ratings.date_drank).all()
        collection = Collection.query.order_by(Collection.date_added).all()
        return render_template('index.html', ratings=ratings, collection=collection)


""" @app.route('/delete/<int:id>')
def delete(id):
    task_to_delete = Todo.query.get_or_404(id)

    try:
        db.session.delete(task_to_delete)
        db.session.commit()
        return redirect('/')
    except:
        return 'There was a problem deleting that task'

@app.route('/update/<int:id>', methods=['GET', 'POST'])
def update(id):
    task = Todo.query.get_or_404(id)

    if request.method == 'POST':
        task.content = request.form['content']

        try:
            db.session.commit()
            return redirect('/')
        except:
            return 'There was an issue updating your task'

    else:
        return render_template('update.html', task=task)
 """

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0")