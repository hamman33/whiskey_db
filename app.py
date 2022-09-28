from operator import truediv
from flask import Flask, render_template, url_for, request, redirect
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///whiskey.db'
db = SQLAlchemy(app)

class Ratings(db.Model):
    rating_num = db.Column(db.Integer, primary_key=True)
    bottle_name = db.Column(db.String(200))
    rating = db.Column(db.Integer, nullable=False)
    drinker = db.Column(db.String(50))
    date_drank = db.Column(db.DateTime, default=datetime.utcnow)
    blind = db.Column(db.Boolean, default=False)

    def __repr__(self):
        return '<whiskey %r>' % self.whiskey

class Collection(db.Model):
    bottle_name = db.Column(db.String(200), primary_key=True)
    whiskey_type = db.Column(db.String(50), default=0)
    proof = db.Column(db.Float, default=0)
    avg_rating = db.Column(db.Float, default=0)
    price = db.Column(db.Float, default=0)

    def __repr__(self):
        return '<bottle %r>' % self.bottle

@app.route('/', methods=['POST', 'GET'])
def index():
    collection = Collection.query.order_by(Collection.whiskey_type, Collection.bottle_name).all()
    return render_template('index.html', collection=collection)

@app.route('/collection', methods=['POST', 'GET'])
def collection():
    print(request.method)
    if request.method == 'POST':
        bottle_name = request.form['bottle_name']
        w_type = request.form['type']
        proof = request.form['proof']
        new_bottle = Collection(bottle_name=bottle_name, whiskey_type=w_type, proof=proof)
        try:
            db.session.add(new_bottle)
            db.session.commit()
            return redirect('/collection')
        except:
            return 'There was an issue adding to your collection'
    else:
        ratings = Ratings.query.order_by(Ratings.date_drank).all()
        collection = Collection.query.order_by(Collection.whiskey_type, Collection.bottle_name).all()
        return render_template('collection.html', ratings=ratings, collection=collection)

@app.route('/ratings', methods=['POST', 'GET'])
def ratings():
    print(request.method)
    if request.method == 'POST':
        #do nothing
        pass
    else:
        ratings = Ratings.query.order_by(Ratings.date_drank).all()
        collection = Collection.query.order_by(Collection.whiskey_type, Collection.bottle_name).all()
        return render_template('ratings.html', ratings=ratings, collection=collection)


@app.route('/data')
def data():
    bot_dct = {i: Collection.query.all()[i].bottle for i in range(len(Collection.query.all()))}
    
    return bot_dct

def proof_round(p):
    if p.is_integer():
        return int(p)
    return p

@app.route('/rate/<string:b_name>', methods=['GET', 'POST'])
def rate(b_name):
    bottle_obj = Collection.query.get_or_404(b_name)
    if request.method == 'POST':
        rating = request.form['rating']
        name = request.form['name']
        blind = request.form['blind']

        if(blind == 'on'):
            blind = True
        else:
            blind = False

        try:
            rat = float(rating)
        except:
            return 'Please enter a number from 0-10'

        if(float(rating) > 10 or float(rating) < 0):
            return 'Please enter a number from 0-10'
        
        new_rating = Ratings(rating_num=max({Ratings.query.all()[i].rating_num for i in range(len(Ratings.query.all()))} or [0])+1, bottle_name=bottle_obj.bottle_name, rating=float(rating), drinker=name, blind=blind)
        try:
            db.session.add(new_rating)
            db.session.commit()
        except:
            return 'There was an issue adding the rating'
                
        #update avg ratings
        all_ratings = Ratings.query.filter_by(bottle_name=b_name).all()
        all_ratings_num = [rate.rating for rate in all_ratings]
        print(all_ratings_num)

        bottle_obj.avg_rating = round((sum(all_ratings_num)/len(all_ratings_num)), 2)
        db.session.commit()

        return redirect('/')
    else:
        return render_template('rate.html', bottle=bottle_obj)

@app.route('/deletebottle/<string:id>')
def deletebottle(id):
    bottle_obj = Collection.query.get_or_404(id)
    ratings = Ratings.query.filter_by(bottle_id=id)

    try:
        db.session.delete(bottle_obj)
        for rat in ratings:
            db.session.delete(rat)
        db.session.commit()
        return redirect('/collection')
    except:
        return 'There was a problem deleting the bottle'

@app.route('/editbottle/<string:id>', methods=['GET', 'POST'])
def editbottle(id):
    bottle = Collection.query.get_or_404(id)
    print("here")
    if request.method == 'POST':
        bottle.bottle = request.form['bottle']
        bottle.whiskey_type = request.form['type']
        bottle.proof = request.form['proof']
        bottle.owner = request.form['owner']
        try:
            db.session.commit()
            return redirect('/collection')
        except:
            return 'There was a problem editing the bottle'
    else:
        return render_template("editbottle.html", bottle=bottle)

@app.route('/deleterating/<string:id>')
def deleterating(id):
    rating_obj = Ratings.query.get_or_404(id)

    try:
        db.session.delete(rating_obj)
        db.session.commit()
        return redirect('/ratings')
    except:
        return 'There was a problem deleting the rating'
 

def main():
    app.run(debug=True, host="0.0.0.0")
    db.create_all()

if __name__ == "__main__":
    main()
