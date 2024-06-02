#ifndef COURT_H
#define COURT_H

#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <vector>


using namespace std; 

class Court {
public:
    Court(int numPlayers, int hasReferee);
    ~Court();

    void enter();
    void play();
    void leave();

private:
    int numPlayers;            // Number of players needed for a match
    int hasReferee;            // Indicates if a referee is needed
    int currentPlayers;        // Current number of players in the court
    pthread_t refereeId;       // ID of the referee, if present
    bool matchInProgress;      // Indicates if a match is currently in progress
    sem_t printMutex;          // Semaphore for print mutual exclusion
    sem_t currentPlayerMutex;  // Semaphore for currentPlayer mutual exclusion
    sem_t mathMutex;           // Semaphore for math mutual exclusion
    sem_t canEnter;            // Semaphore to control entry
    sem_t canLeave;            // Semaphore for orderly leaving

    void checkStartMatch();
    void checkEndMatch();
};

Court::Court(int numPlayers, int hasReferee) : numPlayers(numPlayers), hasReferee(hasReferee), currentPlayers(0), matchInProgress(false), refereeId(0) {
    if (numPlayers <= 0 || (hasReferee != 0 && hasReferee != 1)) {
            throw invalid_argument("An error occured.");
    }

    sem_init(&printMutex, 0, 1);
    sem_init(&mathMutex, 0, 1);
    sem_init(&currentPlayerMutex, 0, 1);
    sem_init(&canEnter, 0, numPlayers + hasReferee);
    sem_init(&canLeave, 0, 0);
}

Court::~Court() {
    sem_destroy(&printMutex);
    sem_destroy(&mathMutex);
    sem_destroy(&currentPlayerMutex);
    sem_destroy(&canEnter);
    sem_destroy(&canLeave);
}

void Court::enter() {
    sem_wait(&printMutex);
    cout << "Thread ID: " << pthread_self() << ", I have arrived at the court." << endl;
    sem_post(&printMutex);


    sem_wait(&canEnter); // dropping canEnter count by one
    
    sem_wait(&currentPlayerMutex);
    sem_wait(&mathMutex);
    currentPlayers++;
    sem_post(&mathMutex);

    checkStartMatch();

    sem_post(&currentPlayerMutex);
}

void Court::leave() {
    if(matchInProgress) {
        if (hasReferee && pthread_equal(pthread_self(), refereeId)) {
        
        sem_wait(&printMutex);
        cout << "Thread ID: " << pthread_self() << ", I am the referee and now, match is over. I am leaving." << endl;
        sem_post(&printMutex);
        
        refereeId = 0;  // Reset referee ID
        
        sem_wait(&mathMutex);
        for(int i = 0; i < numPlayers; i++)
            sem_post(&canLeave); // Allow players to start leaving
        sem_post(&mathMutex);
        
        } else {
            if(hasReferee)
                sem_wait(&canLeave); // Players wait until referee leaves
        
            sem_wait(&printMutex);
            cout << "Thread ID: " << pthread_self() << ", I am a player and now, I am leaving." << endl;
            sem_post(&printMutex);
        }

    } else {
        sem_wait(&printMutex);
        cout << "Thread ID: " << pthread_self() << ", I was not able to find a match and I have to leave." << endl;
        sem_post(&printMutex);
        sem_post(&canEnter); // since this players leaves without playing a match, it should allow one new player to enter
    }
    
    sem_wait(&currentPlayerMutex);

    sem_wait(&mathMutex);
    currentPlayers--;
    sem_post(&mathMutex);

    checkEndMatch();

    sem_post(&currentPlayerMutex);

}

void Court::checkStartMatch() {
    if (!matchInProgress && ((hasReferee && currentPlayers == numPlayers + 1) || (!hasReferee && currentPlayers == numPlayers))) {
        matchInProgress = true;
        sem_wait(&printMutex);
        cout << "Thread ID: " << pthread_self() << ", There are enough players, starting a match." << endl;
        sem_post(&printMutex);
        
        if (hasReferee) {
            refereeId = pthread_self(); // Assign current thread as referee
        }
    } else {
        sem_wait(&printMutex);
        cout << "Thread ID: " << pthread_self() << ", There are only " << currentPlayers << " players, passing some time." << endl;
        sem_post(&printMutex);
    }
}

void Court::checkEndMatch() {
    if (matchInProgress && currentPlayers == 0) {
        matchInProgress = false;
        
        sem_wait(&printMutex);
        cout << "Thread ID: " << pthread_self() << ", everybody left, letting any waiting people know." << endl;
        sem_post(&printMutex);
        
        sem_wait(&mathMutex);
        for(int i = 0; i < numPlayers + hasReferee; i++)
            sem_post(&canEnter); // allowing new players to enter to the court
        sem_post(&mathMutex);
    }
}

#endif // COURT_H
