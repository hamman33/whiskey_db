from operator import truediv
from flask import Flask, render_template, url_for, request, redirect
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
from os.path import exists
from google_images_download import google_images_download
import os
import pytz

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///whiskey.db'
db = SQLAlchemy(app)

class Ratings(db.Model):
    rating_num = db.Column(db.Integer, primary_key=True)
    bottle_id = db.Column(db.Integer, nullable=False)
    rating = db.Column(db.Integer, nullable=False)
    drinker = db.Column(db.String(50))
    date_drank = db.Column(db.DateTime, default=datetime.utcnow())
    blind = db.Column(db.Boolean, default=False)


class Collection(db.Model):
    bottle_id = db.Column(db.Integer, primary_key=True)
    bottle_name = db.Column(db.String(200))
    whiskey_type = db.Column(db.String(50), default=0)
    proof = db.Column(db.Float, default=0)
    avg_rating = db.Column(db.Float, default=0)
    num_ratings = db.Column(db.Integer, default=0)

    def __repr__(self):
        return '<bottle %r>' % self.bottle

@app.route('/', methods=['POST', 'GET'])
def index():
    #print(Ratings.query.all().rating_num)
    calcRatings()
    collection = Collection.query.order_by(Collection.whiskey_type, Collection.bottle_name).all()
    return render_template('index.html', collection=collection)


def calcRatings():
    for i in range(len(Collection.query.all())):
        id = Collection.query.all()[i].bottle_id    
        bottle_obj = Collection.query.get_or_404(id)
        all_ratings = Ratings.query.filter_by(bottle_id=id).all()
        bottle_obj.num_ratings = len(all_ratings)
    db.session.commit()
    return

@app.route('/collection', methods=['POST', 'GET'])
def collection():
    print(request.method)
    if request.method == 'POST':
        bottle_id = max({Collection.query.all()[i].bottle_id for i in range(len(Collection.query.all()))} or [0])+1
        bottle_name = request.form['bottle_name']
        w_type = request.form['type']
        proof = request.form['proof']
        new_bottle = Collection(bottle_id=bottle_id, bottle_name=bottle_name, whiskey_type=w_type, proof=proof)
        try:
            db.session.add(new_bottle)
            db.session.commit()
            return redirect('/collection')
        except:
            return 'There was an issue adding to your collection'
    else:
        collection = Collection.query.order_by(Collection.whiskey_type, Collection.bottle_name).all()
        return render_template('collection.html', collection=collection)

@app.route('/ratings', methods=['POST', 'GET'])
def ratings():
    print(request.method)
    if request.method == 'POST':
        #do nothing
        pass
    else:
        ratings = db.session.query(Ratings.rating_num, Ratings.rating, Ratings.drinker, Ratings.date_drank, Ratings.blind, Collection.bottle_name).join(Collection, Ratings.bottle_id==Collection.bottle_id).order_by(Ratings.date_drank.desc()).all()
        return render_template('ratings.html', ratings=ratings)


@app.route('/data')
def data():
    bot_dct = {i: Collection.query.all()[i].bottle for i in range(len(Collection.query.all()))}
    
    return bot_dct

def proof_round(p):
    if p.is_integer():
        return int(p)
    return p

@app.route('/rate/<int:id>', methods=['GET', 'POST'])
def rate(id):
    bottle_obj = Collection.query.get_or_404(id)
    all_ratings = Ratings.query.filter_by(bottle_id=id).all()
    all_ratings_num = [rate.rating for rate in all_ratings]

    if request.method == 'POST':
        rating = request.form['rating']
        name = request.form['name']
        blind = request.form.get('blind')
        print(blind)
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
        rate_num_new = max(r.rating_num for r in db.session.query(Ratings.rating_num))+1
        new_rating = Ratings(rating_num=rate_num_new, bottle_id=bottle_obj.bottle_id, rating=float(rating), drinker=name, blind=blind)
        try:
            db.session.add(new_rating)
            #db.session.commit()
        except:
            return 'There was an issue adding the rating'      
        #update avg ratings
        
        bottle_obj.avg_rating = round((sum(all_ratings_num)/len(all_ratings_num)), 1)
        bottle_obj.num_ratings = len(all_ratings_num)
        db.session.commit()

        return redirect('/')
    else:
        image_path = "../static/images/" + str(bottle_obj.bottle_name).replace(" ", "_").lower() + ".jpg"
        if(not exists(image_path[1:])):
            botname = str(bottle_obj.bottle_name).replace(" ", "_").lower()
            args = {}
            args["keywords"] = botname + " " + bottle_obj.whiskey_type + " whiskey"
            args["limit"] = 1
            args["format"] = "jpg"
            args["output_directory"] = "static"
            args["image_directory"] = "images"
            args["aspect_ratio"] = "tall"
            args["time"] = "past-year"

            try:
                response = google_images_download.googleimagesdownload()
                absolute_image_paths = response.download(args)
                os.rename(absolute_image_paths[0][args["keywords"]][0], "./static/images/" + botname + ".jpg")
            except:
                print("cant find bottle image")

        return render_template('rate.html', bottle=bottle_obj, image=image_path, ratings=all_ratings)

@app.route('/deletebottle/<int:id>')
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

@app.route('/editbottle/<int:id>', methods=['GET', 'POST'])
def editbottle(id):
    bottle = Collection.query.get_or_404(id)
    if request.method == 'POST':
        bottle.bottle_name = request.form['bottle']
        bottle.whiskey_type = request.form['type']
        bottle.proof = request.form['proof']
        try:
            db.session.commit()
            return redirect('/collection')
        except:
            return 'There was a problem editing the bottle'
    else:
        return render_template("editbottle.html", bottle=bottle)

@app.route('/deleterating/<int:id>')
def deleterating(id):
    print(id)
    rating_obj = Ratings.query.get_or_404(id)

    try:
        db.session.delete(rating_obj)
        db.session.commit()
        return redirect('/ratings')
    except:
        return 'There was a problem deleting the rating'
 

def main():
    app.run(debug=False, host="0.0.0.0")
    db.create_all()

if __name__ == "__main__":
    main()
