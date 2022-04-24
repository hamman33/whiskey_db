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
    whiskey_type = db.Column(db.String(200), default=0)
    proof = db.Column(db.Float, default=0)
    avg_rating = db.Column(db.Float, default=0)
    date_added = db.Column(db.DateTime, default=datetime.utcnow)
    date_killed = db.Column(db.DateTime)

    def __repr__(self):
        return '<bottle %r>' % self.bottle

@app.route('/', methods=['POST', 'GET'])
def index():
    print(request.method)
    if request.method == 'POST':
        bottle = request.form['bottle']
        w_type = request.form['type']
        proof = request.form['proof']
        new_bottle = Collection(bottle=bottle, whiskey_type=w_type, proof=proof)
        try:
            db.session.add(new_bottle)
            db.session.commit()
            return redirect('/')
        except:
            return 'There was an issue adding to your collection'
    else:
        ratings = Ratings.query.order_by(Ratings.date_drank).all()
        collection = Collection.query.order_by(Collection.date_added).all()
        return render_template('index.html', ratings=ratings, collection=collection)


@app.route('/data')
def data():
    bot_dct = {i: Collection.query.all()[i].bottle for i in range(len(Collection.query.all()))}
    
    return bot_dct

def proof_round(p):
    if p.is_integer():
        return int(p)
    return p

@app.route('/rate/<string:id>', methods=['GET', 'POST'])
def rate(id):
    print("here")
    bottle = Collection.query.get_or_404(id)

    if request.method == 'POST':
        task.content = request.form['content']

        print("here1")
        try:
            db.session.commit()
            return redirect('/')
        except:
            return 'There was an issue updating your task'

    else:
        print("here2")
        return render_template('rate.html', bottle=bottle)

""" @app.route('/delete/<int:id>')
def delete(id):
    task_to_delete = Todo.query.get_or_404(id)

    try:
        db.session.delete(task_to_delete)
        db.session.commit()
        return redirect('/')
    except:
        return 'There was a problem deleting that task'
 """

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0")
    db.create_all()