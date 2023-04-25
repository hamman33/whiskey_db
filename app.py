from operator import truediv
from flask import Flask, render_template, url_for, request, redirect, flash
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
from os.path import exists
from werkzeug.utils import secure_filename
import os
import json

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///whiskey.db'
app.config['UPLOAD_FOLDER'] = './static/images/'
app.secret_key = "super secret key"
ALLOWED_EXTENSIONS = set(['txt', 'pdf', 'png', 'jpg', 'jpeg', 'gif'])
db = SQLAlchemy(app)

class Ratings(db.Model):
    rating_num = db.Column(db.Integer, primary_key=True)
    bottle_id = db.Column(db.Integer, nullable=False)
    rating = db.Column(db.Float, nullable=False)
    drinker = db.Column(db.String(50))
    date_drank = db.Column(db.DateTime, default=datetime.now)
    blind = db.Column(db.Boolean, default=False)
    def __repr__(self):
        return '<rating %r>' % self.rating


class Collection(db.Model):
    bottle_id = db.Column(db.Integer, primary_key=True)
    bottle_name = db.Column(db.String(200))
    whiskey_type = db.Column(db.String(50), default=0)
    proof = db.Column(db.Float, default=0)
    avg_rating = db.Column(db.Float, default=0)
    num_ratings = db.Column(db.Integer, default=0)

    def __repr__(self):
        return '<bottle %r>' % self.bottle
    
class Users(db.Model):
    user_id = db.Column(db.Integer, primary_key=True)
    user_name = db.Column(db.String(50))
    num_ratings = db.Column(db.Integer, default=0)
    num_blinds = db.Column(db.Integer, default=0)
    avg_rating = db.Column(db.Float, default=0)
    avg_blind = db.Column(db.Float, default=0)
    num_bottles = db.Column(db.Integer, default=0)
    
class Owners(db.Model):
    owner_id = db.Column(db.Integer, primary_key=True)
    bottle_id = db.Column(db.Integer, nullable=False)
    user_id = db.Column(db.Integer, nullable=False)
    owns = db.Column(db.Boolean, default=True)
    

@app.route('/', methods=['POST', 'GET'])
def index():
    tBotNum = db.session.query(Collection).count()
    tRatNum = db.session.query(Ratings).count()
    newRatings = db.session.query(Ratings.rating, Ratings.drinker, Ratings.date_drank, Collection.bottle_name).join(Collection, Ratings.bottle_id==Collection.bottle_id).order_by(Ratings.date_drank.desc()).limit(5).all()
    
    collection = Collection.query.order_by(Collection.whiskey_type, Collection.bottle_name).all()
    return render_template('index.html', collection=collection, tBotNum=tBotNum, tRatNum=tRatNum, newRatings=newRatings)


@app.route('/collection', methods=['POST', 'GET'])
def collection():
    print(request.method)
    if request.method == 'POST':
        bottle_id = max(bot.bottle_id for bot in Collection.query.all())+1
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

@app.route('/users', methods=['POST', 'GET'])
def users():
    if request.method == 'POST':
        #do nothing
        pass
    else:
        users = db.session.query(Users.user_id, Users.user_name, Users.avg_rating, Users.num_ratings, Users.num_bottles).order_by(Users.user_name).all()
        return render_template('users.html', users=users)
    
@app.route('/user/<int:id>', methods=['POST', 'GET'])
def user(id):
    user_obj = Users.query.get_or_404(id)
    if request.method == 'POST':
        #do nothing
        pass
    else:
        ratings = db.session.query(Ratings.rating_num, Ratings.rating, Ratings.drinker, Ratings.date_drank, Ratings.blind, Collection.bottle_name).filter(Ratings.drinker==user_obj.user_name).join(Collection, Ratings.bottle_id==Collection.bottle_id).order_by(Ratings.date_drank.desc()).all()
        return render_template('user.html', user=user_obj, ratings=ratings)


@app.route('/recentratings')
def recentratings():
    newRatings = db.session.query(Ratings.rating, Ratings.drinker, Collection.bottle_name).join(Collection, Ratings.bottle_id==Collection.bottle_id).order_by(Ratings.date_drank.desc()).limit(5).all()
    rating_string = ""
    for rat in newRatings:
        rating_string = rating_string + rat.bottle_name + "|" + rat.drinker +  "|" + str(rat.rating) + ";"
    
    return rating_string

def proof_round(p):
    if p.is_integer():
        return int(p)
    return p
 
@app.route('/rate/<int:id>', methods=['GET', 'POST'])
def rate(id):
    bottle_obj = Collection.query.get_or_404(id)

    if request.method == 'POST':
        rating = request.form['rating']
        name = request.form['name'].replace(" ", "")
        blind = request.form.get('blind')
    
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
            db.session.commit()
        except:
            return 'There was an issue adding the rating'      
        
        #update avg ratings
        updateBottleRating(id)
        updateUserRatings(name) 

        return redirect('/')
    else:
        image_path = "../static/images/" + str(bottle_obj.bottle_name).replace(" ", "_").lower() + ".jpg"
        if(not exists(image_path[1:])):
            image_path = ''
                
        all_ratings = Ratings.query.filter_by(bottle_id=id).all()
        return render_template('rate.html', bottle=bottle_obj, image=image_path, ratings=all_ratings)

@app.route('/uploadpicture/<int:id>', methods=['GET', 'POST'])
def uploadpicture(id):
    bottle_obj = Collection.query.get_or_404(id)
    if request.method == 'POST':
        # Upload file flask
        uploaded_img = request.files['img']
        if uploaded_img.filename == '':
            print("here2")
            flash('Please select a valid file')
            return redirect(request.url)
        # Extracting uploaded data file name
        botname = str(bottle_obj.bottle_name).replace(" ", "_").lower() + str('.jpg')
        img_filename = secure_filename(uploaded_img.filename)
        # Upload file to database (defined uploaded folder in static path)
        uploaded_img.save(os.path.join(app.config['UPLOAD_FOLDER'], botname))
 

    return redirect('/rate/'+str(id))

def allowed_file(filename):     
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

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
    except:
        return 'There was a problem deleting the rating'
    
    updateBottleRating(rating_obj.bottle_id)
    updateUserRatings(rating_obj.drinker)

    return redirect('/rate/'+str(rating_obj.bottle_id))

def updateBottleRating(id):
    bottle_obj = Collection.query.get_or_404(id)
    all_ratings = Ratings.query.filter_by(bottle_id=id).all()
    all_ratings_num = [rate.rating for rate in all_ratings]
        
    bottle_obj.avg_rating = round((sum(all_ratings_num)/len(all_ratings_num)), 1)
    bottle_obj.num_ratings = len(all_ratings_num)

    db.session.commit()


def updateUserRatings(name):
    user = db.session.query(Users.user_id).filter(Users.user_name==name).all()
    if len(user)==0:
        addUser(name)
        updateUserRatings(name)
    else:
        u = Users.query.get_or_404(user[0].user_id)
        ratings = db.session.query(Ratings.rating, Ratings.drinker, Ratings.blind).filter(Ratings.drinker==name).all()
        bratings = db.session.query(Ratings.rating, Ratings.drinker, Ratings.blind).filter(Ratings.drinker==name, Ratings.blind==True).all()
        rats = [rate.rating for rate in ratings]
        brats = [rate.rating for rate in bratings]
        u.num_ratings = len(rats)
        u.num_blinds = len(brats)
        u.avg_rating = round((sum(rats)/len(rats)), 2)
        if(len(brats)!=0):
            u.avg_blind = round((sum(brats)/len(brats)), 2)
        db.session.commit()
    
    
def addUser(name):
    uid = max(user.user_id for user in Users.query.all())+1
    newuser = Users(user_id=uid, user_name=name)
    db.session.add(newuser)
    db.session.commit()

def main():
    app.run(debug=False, host="0.0.0.0")
    db.create_all()

if __name__ == "__main__":
    main()
