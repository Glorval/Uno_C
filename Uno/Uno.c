#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#define DECKLENGTH 108//108 normal, 76 numbers only, 100 no wilds
#define NUMBERDEALT 7
#define SHUFFLETHOROUGHNESS 5000
#define MAXNUMBEROFPLAYERS 4//Specifically, max number of AIs


int dataCorruption = 0;//Global flag for marking if there has been data corruption.
int currentTopdeck = 0;

struct cardData {//Holds the data of each card
	char data[2];//Holds the data of a single card
	int cardCount;//Used for players
	int returnLegalCheck;//Used for if the AI can make a legal play or not//MADE OBSOLETE IN AI VERSION 1.2
	int drawing;//Used in 1.2 AI to tell the main if the AI needs to draw.
};
typedef struct cardData card;

struct aiCluster {//Holds all the AI's
	card Player[DECKLENGTH];
};
typedef struct aiCluster ai;

FILE* debugDump;


void dataShifter(card input, card *writeto);//Easy way of slapping the data from one card entry into another
void shuffle(card deck[DECKLENGTH]);//Shuffles the current deck
void printDeck(card deck[DECKLENGTH]);//Prints the deck, debug feature
void print(card deck);//Prints the card (Less typing in code lol)
void detailedPrint(card deck);//Prints the card readibly, will print Colour then Number, or just Wild or Wild Pluf Four
void clearscreen();//'Clears' the screen
int isThisLegal(card inputCard, card cardInPlay);//Returns 1 for a regularly legal play, 2 for wild,  3 for wild plus four, zero for not legal.
int checkAgainstHand(card playerInput, card playerHand[DECKLENGTH]);//Returns 1 if its in their hand, 0 if not
void removeCardFromHand(card CardToBeRemoved, card Hand[DECKLENGTH], card* HandPassalong);//Removes the designated card from the designated hand
int updateCurrentPlayer(int currentPlayer, int direction, int playerCount);//Returns the new current player value
int drawCard(card deck[DECKLENGTH], card* PlayerDrawEntry, card* PlayerFirstEntry, int currentTopdeck);//Easier way to draw a card than having the code block over and over again
void displayOthersCardCount(int playerCount, ai AI[MAXNUMBEROFPLAYERS]);
void intToPrint(int input, int capitalized);//Takes the input int and prints it.
char cardsOfColourCounter(card Hand[DECKLENGTH]); //Takes the hand input and counts the # of cards of each colour, returning the colour with the most cards
card aiV1(card Hand[DECKLENGTH], card Deck[DECKLENGTH], card cardInPlay);//First version of AI, capable of a pure numbers game//Ended capable of numbers/specials, no wilds
card aiV1_2(card Hand[DECKLENGTH], card cardInPlay, int currentPlayer, int direction);//Version 1.2 of AI, capable of full game with no prediction or proper thought.


void main() {
	srand(time(0));


	//create the deck variable
	card deck[DECKLENGTH];//The array that contains the deck

	//Throughout the game variables
	int playerCount = 0;//How many AI
	int gameRunning = 1;//Is the game set to continue in the loop after this one finishes?
	card cardInPlay;//The current card in play
	cardInPlay.data[0] = 'E';
	cardInPlay.data[1] = 'E';
	card PlayerZero[DECKLENGTH];//, PlayerOne[DECKLENGTH], PlayerTwo[DECKLENGTH], PlayerThree[DECKLENGTH], PlayerFour[DECKLENGTH];
	ai AI[MAXNUMBEROFPLAYERS + 1];//Just ignore AI[0]...


	//_________________________________________
	//PRE GAME SETUP
	//_________________________________________





		//fill the data of the deck
		FILE* deckfile;
		deckfile = fopen("Deck.txt", "r");
		for (int fill = 0; fill < DECKLENGTH; fill++) {
			fscanf(deckfile, "%s", &deck[fill].data);
		}
		//End of deck filling

	
		shuffle(deck);
		//printDeck(deck);//Debug printing of the entire deck



		printf("\n\n\n\n");//Ask the player and store how many AIs they want
		printf("How many other players would you like? Max 4, min 1		");
		scanf("%d", &playerCount);
		clearscreen();
		//dealing cards to everyone
		for (int dealtCards = 0; dealtCards < NUMBERDEALT; dealtCards++) {
			for (int currentPlayer = 0; currentPlayer <= playerCount + 1; currentPlayer++) {//Playercount + 1 to account for the user themselves
				switch (currentPlayer) {
					case 0:
						PlayerZero[dealtCards].data[0] = deck[currentTopdeck].data[0];//Write the data from the deck to their hand
						PlayerZero[dealtCards].data[1] = deck[currentTopdeck].data[1];
						currentTopdeck++;//Increment because card has been drawn from deck to a players hand
						break;
					case 1:
						AI[1].Player[dealtCards].data[0] = deck[currentTopdeck].data[0];//All the rest are the same
						AI[1].Player[dealtCards].data[1] = deck[currentTopdeck].data[1];
						currentTopdeck++;
						break;
					case 2:
						AI[2].Player[dealtCards].data[0] = deck[currentTopdeck].data[0];
						AI[2].Player[dealtCards].data[1] = deck[currentTopdeck].data[1];
						currentTopdeck++;
						break;
					case 3:
						AI[3].Player[dealtCards].data[0] = deck[currentTopdeck].data[0];
						AI[3].Player[dealtCards].data[1] = deck[currentTopdeck].data[1];
						currentTopdeck++;
						break;
					case 4:
						AI[4].Player[dealtCards].data[0] = deck[currentTopdeck].data[0];
						AI[4].Player[dealtCards].data[1] = deck[currentTopdeck].data[1];
						currentTopdeck++;
						break;
				}
			}
		}

		//Setting the card count of each player, even if they arent in the game it doesnt matter if they are set to 7
		PlayerZero[0].cardCount = 7;
		AI[1].Player[0].cardCount = 7;
		AI[2].Player[0].cardCount = 7;
		AI[3].Player[0].cardCount = 7;
		AI[4].Player[0].cardCount = 7;

		
		dataShifter(deck[currentTopdeck], &cardInPlay);//Stores the next card as the card in play, thus kicking off the game
		currentTopdeck++;//Account for playing a card off the deck

	//_________________________________________
	//END OF PRE GAME SETUP
	//_________________________________________

	

	//game loop
	int currentPlayer = 0;//Keeps track of the current player
	int direction = 1;//Keeps track of which direction and whether to increase or decrease the current player at the end of each players turn, 1 is normal -1 is antinormal.
	int drawTwod = 0;
	int skipped = 0;
	int reversed = 0;
	int drawFourd = 0;


	debugDump = fopen("dump.txt", "w");

	while (gameRunning == 1) {
		//Player always goes first

		fprintf(debugDump, "Start of Loop\n");//DEBUG RECORDING


		
		if (currentPlayer == 0) {//Its the players turn

			fprintf(debugDump, "Player Zeros turn, direction %d\n", direction);//DEBUG RECORDING

			//START OF TURN LOGIC
			if (skipped == 1) {//Did we get skipped?
				
				printf("You were skipped\n");
				currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);//End of turn, moving on
				skipped = 0;
			}else if (drawTwod == 1) {//Did we get plus 2?
				fprintf(debugDump, "Player Zero was +2d.\n");//DEBUG RECORDING
				printf("You were Draw Twoed\n");
				//Drawing 2
				currentTopdeck = drawCard(deck, &PlayerZero[PlayerZero[0].cardCount], &PlayerZero[0], currentTopdeck);
				currentTopdeck = drawCard(deck, &PlayerZero[PlayerZero[0].cardCount], &PlayerZero[0], currentTopdeck);
				//End of drawing two
				currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);//End of turn, move on
				drawTwod = 0;
			}else if(drawFourd == 1){
				fprintf(debugDump, "Player Zero was +4d\n");
				printf("You were Draw Foured\n");
				//Drawing 4
				currentTopdeck = drawCard(deck, &PlayerZero[PlayerZero[0].cardCount], &PlayerZero[0], currentTopdeck);
				currentTopdeck = drawCard(deck, &PlayerZero[PlayerZero[0].cardCount], &PlayerZero[0], currentTopdeck);
				currentTopdeck = drawCard(deck, &PlayerZero[PlayerZero[0].cardCount], &PlayerZero[0], currentTopdeck);
				currentTopdeck = drawCard(deck, &PlayerZero[PlayerZero[0].cardCount], &PlayerZero[0], currentTopdeck);
				//End of drawing 4
				currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);//End of turn, move on
				drawFourd = 0;
			}else{//Normal Turn
				//END OF START OF TURN LOGIC
				
				printf("\nIt is your turn, you have %d cards, and the current card in play is ", PlayerZero[0].cardCount);
				detailedPrint(cardInPlay);
				printf("\n\n");




				//DISPLAY OTHER PLAYERS CARD COUNT
				displayOthersCardCount(playerCount, AI);
				//END OF DISPLAYING OTHER PLAYERS CARD COUNT




				//PRINT PLAYERS HAND
				printf("Your hand:\n");//Printing the players entire hand out nicely
				for (int currentCard = 0; currentCard < PlayerZero[0].cardCount; currentCard++) {
					detailedPrint(PlayerZero[currentCard]);
					printf(", ");
				}
				printf("\n\n");
				//END OF PRINTING PLAYERS HAND



				//TAKE PLAYER INPUT AND FINISH THE TURN
				int legalPlay = 0;
				int legalishPlay = 0;
				card PlayerInput;
				while (legalPlay < 1 || legalishPlay < 1) {//So that if a player puts in an illegal play it just resets
					printf("What card would you like to play?\n");
					scanf("%c", &PlayerInput.data[0]);//Takes out the trash
					scanf("%c", &PlayerInput.data[0]);
					scanf("%c", &PlayerInput.data[1]);
					printf("\n\n\n\n\n\n\n\n\n\n\n\n\n");
					if (PlayerInput.data[0] == 'D') {//The player wishes to draw a card
						currentTopdeck = drawCard(deck, &PlayerZero[PlayerZero[0].cardCount], &PlayerZero[0], currentTopdeck);
						printf("You drew and are up to %i cards\n\n", PlayerZero[0].cardCount);
						//legacy //I really should write what made these obsolete
						//dataShifter(deck[currentTopdeck], &PlayerZero[PlayerZero[0].cardCount]);//Copy the data from the current top of the deck into the next empty space in the players hand
						//currentTopdeck++;//Keep track of the top of the deck
						//PlayerZero[0].cardCount++;//Keep track of the players hand size
						break;//Leave the while loop here because we just drew, we dont get to play now under these rules
					}
					else if(PlayerInput.data[0] == 'E'){
						gameRunning = 0;
						fclose(debugDump);
						break;
					}else {
						legalishPlay = isThisLegal(PlayerInput, cardInPlay);//Get if its a physically legal card to play
						if (legalishPlay == 0){//Doesnt match so illegal play, this while loop will loop
							printf("\nDoesn't match card in play\n");
						}
						if (checkAgainstHand(PlayerInput, PlayerZero) == 1) {
							legalPlay = 1;//Its in their hand so this part is a legal play
							
							printf("You played ");
							detailedPrint(PlayerInput);
							printf(" on a ");
							detailedPrint(cardInPlay);
							printf("\n\n");
						}else {//Doesnt match a card in their hand so illegal play, this while loop will loop
							printf("\nIsn't a card in your hand\n");
						}
					}
				}
				if (legalPlay == 1 && legalishPlay >= 1) {//If its a legal play, do below, otherwise the player has drawn a card and we move on.
					//Now we have a player input that is a legal play and from their hand
					if(PlayerInput.data[0] == 'S') skipped = 1;		//Did we play a skip	
					if(PlayerInput.data[0] == 'R') direction = direction * -1;//Did we reverse
					if(PlayerInput.data[0] == 'T') drawTwod = 1;//Did we play a draw two
					if(PlayerInput.data[0] == 'F') drawFourd = 1;//Did we play a draw four

					dataShifter(PlayerInput, &cardInPlay);
					if (PlayerInput.data[0] == 'F' || PlayerInput.data[0] == 'W') {
						PlayerInput.data[1] = 'w';
						removeCardFromHand(PlayerInput, PlayerZero, &PlayerZero);
					}
					else {
						removeCardFromHand(PlayerInput, PlayerZero, &PlayerZero);
					}
					PlayerZero[0].cardCount--;//Make sure to keep track of the players cards
				}



				if (PlayerZero[0].cardCount == 0) {//End of the game check
					clearscreen();
					printf("Congratulations, you have won!\n");
					gameRunning = 0;
					currentPlayer = -1;
				}
				else if(gameRunning == 1){
					currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);//End of normal turn
					fprintf(debugDump, "Normal turn, played %c%c. Direction: %d, new turn %d\n", PlayerInput.data[0], PlayerInput.data[1], direction, currentPlayer);//DEBUG RECORDING
				}
				
				


			}




		}//End of the players turn

		//AI BLOCK
		//It has to be someones turn now, just either did the players turn or its not their turn yet, so it now HAS to be SOME ai's turn.

			//AI
		if (gameRunning == 1) {//Both in case the player wins and to get the rest of the code out one
			if (skipped == 1) {
				printf("AI ");
				intToPrint(currentPlayer, 1);
				printf(" was Skipped.\n");
				fprintf(debugDump, "AI %d was skipped.\n", currentPlayer);
				skipped = 0;
				currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);
			}
			else if (drawTwod == 1) {
				printf("AI ");
				intToPrint(currentPlayer, 1);
				printf(" was Plus Twoed.\n");
				fprintf(debugDump, "AI %d was +2d.\n", currentPlayer);
				drawTwod = 0;
				currentTopdeck = drawCard(deck, &AI[currentPlayer].Player[AI[currentPlayer].Player[0].cardCount], &AI[currentPlayer].Player[0], currentTopdeck);
				currentTopdeck = drawCard(deck, &AI[currentPlayer].Player[AI[currentPlayer].Player[0].cardCount], &AI[currentPlayer].Player[0], currentTopdeck);
				currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);
			}
			else if (drawFourd == 1) {
				printf("AI ");
				intToPrint(currentPlayer, 1);
				printf(" was Plus Foured.\n");
				fprintf(debugDump, "AI %d was +4d.\n", currentPlayer);
				drawFourd = 0;
				for (int counter = 0; counter < 4; counter++) {
					currentTopdeck = drawCard(deck, &AI[currentPlayer].Player[AI[currentPlayer].Player[0].cardCount], &AI[currentPlayer].Player[0], currentTopdeck);
				}
				currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);
			}
			else {
				card AIPlay = aiV1_2(AI[currentPlayer].Player, cardInPlay, currentPlayer, direction);
				if (AIPlay.data[0] != 'D') {//The AI can play a card
					if (AIPlay.data[0] == 'S') skipped = 1;		//Did we play a skip	
					if (AIPlay.data[0] == 'R') direction = direction * -1;//Did we reverse
					if (AIPlay.data[0] == 'T') drawTwod = 1;//Did we play a draw two
					if (AIPlay.data[0] == 'F') drawFourd = 1;//Did we play a +4?
					removeCardFromHand(AIPlay, AI[currentPlayer].Player, &AI[currentPlayer].Player);
					AI[currentPlayer].Player[0].cardCount--;
					dataShifter(AIPlay, &cardInPlay);
				}
				else {//AI is drawing because it cannot play a card
					dataShifter(deck[currentTopdeck], &AI[currentPlayer].Player[AI[currentPlayer].Player[0].cardCount]);//Copy the data from the current top of the deck into the next empty space in the players hand
					currentTopdeck++;//Keep track of the top of the deck
					AI[currentPlayer].Player[0].cardCount++;//Keep track of the players hand size
					printf("AI ");
					intToPrint(currentPlayer, 1);
					printf(" drew and is now up to %d cards.\n", AI[currentPlayer].Player[0].cardCount);
				}

				if (AI[currentPlayer].Player[0].cardCount == 0) {//End of the game check
					clearscreen();
					printf("AI ");
					intToPrint(currentPlayer, 1);
					printf(" has won the game!\n");
					gameRunning = 0;
					currentPlayer = -1;
				}
				else {
					currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);//End of normal turn
				}
			}


		}

		//END OF AI BLOCK


		



	}//END OF LOOP


}


//Prints out the cards raw data
void print(card deck) {//Prints out the cards raw data
	printf("%c%c", deck.data[0], deck.data[1]);
}


//Prints the card readibly, will print Colour then Number, or just Wild or Wild Pluf Four
void detailedPrint(card deck) {// Will print Colour then Number, or just Wild or Wild Pluf Four
	switch (deck.data[1]) {
		case('r'):
			printf("Red ");
			break;
		case('b'):
			printf("Blue ");
			break;
		case('y'):
			printf("Yellow ");
			break;
		case('g'):
			printf("Green ");
			break;
		case('w'):
			//No printing, hand/start wilds handled purely in the other switch
			break;
		default:
			printf("Data corruption noticed when hitting: Detailed Print A");
			dataCorruption = 1;
	}
	
	switch (deck.data[0]) {
		case('0'):
			printf("Zero");
			break;
		case('1'):
			printf("One");
			break;
		case('2'):
			printf("Two");
			break;
		case('3'):
			printf("Three");
			break;
		case('4'):
			printf("Four");
			break;
		case('5'):
			printf("Five");
			break;
		case('6'):
			printf("Six");
			break;
		case('7'):
			printf("Seven");
			break;
		case('8'):
			printf("Eight");
			break;
		case('9'):
			printf("Nine");
			break;
		case('T'):
			printf("Plus Two");
			break;
		case('S'):
			printf("Skip");
			break;
		case('R'):
			printf("Reverse");
			break;
		case('F'):
			printf("Plus Four");
			break;
		case('W'):
			printf("Wild");
			break;
		default:
			printf("Data corruption noticed when hitting: Detailed Print B");
			dataCorruption = 1;
	}
}


//debug feature to print deck
void printDeck(card deck[DECKLENGTH]) {//debug feature to print deck
	for (int counter = 0; counter < DECKLENGTH; counter++) {
		printf("%s\n", deck[counter].data);
	}
}


//'Clears' the screen
void clearscreen() {
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
}

//Easy way of slapping the data from one card entry into another
void dataShifter(card input, card *writeto){//Easy way of slapping the data from one card entry into another
	writeto->data[0] = input.data[0];
	writeto->data[1] = input.data[1];
}


//shuffles the CURRENT DECK, disregards the cards already drawn that still remain in the 'deck' structure array
void shuffle(card deck[DECKLENGTH]) {//shuffles the CURRENT DECK, disregards the cards already drawn that still remain in the 'deck' structure array
	for (int counter = currentTopdeck; counter < SHUFFLETHOROUGHNESS; counter++) {
		int p1 = (rand() % (DECKLENGTH - currentTopdeck));
		int p2 = (rand() % (DECKLENGTH - currentTopdeck));
		card temp = deck[p1];
		deck[p1] = deck[p2];
		deck[p2] = temp;
	}
}

//Returns 1 for a regularly legal play, 2 for wild,  3 for wild plus four, zero for not legal.
int isThisLegal(card inputCard, card cardInPlay) {
	if (inputCard.data[0] == 'W') {
		return(2);
	}else if (inputCard.data[0] == 'F') {
		return(3);
	}else if (inputCard.data[0] == cardInPlay.data[0]) {
		return(1);
	}else if (inputCard.data[1] == cardInPlay.data[1]) {
		return(1);
	}else {
		return(0);
	}
}


//Returns 1 if its in their hand, 0 if not
int checkAgainstHand(card playerInput, card playerHand[DECKLENGTH]){
	for (int counter = 0; counter < playerHand[0].cardCount; counter++) {
		if ((playerInput.data[0] == playerHand[counter].data[0]) && (playerInput.data[1] == playerHand[counter].data[1])) {
			return(1);
		}
		else if (playerInput.data[0] == 'W') {
			return(1);
		}
		else if (playerInput.data[1] == 'F') {
			return(1);//Blessed misspelling reutrn
		}
	}
	return(0);
}



//Removes the designated card from the designated hand
void removeCardFromHand(card CardToBeRemoved, card Hand[DECKLENGTH], card *HandPassalong) {
	int current = 0;
	while ((CardToBeRemoved.data[0] != Hand[current].data[0]) || (CardToBeRemoved.data[1] != Hand[current].data[1])){//Is the current card different at all from the one we're looking for?
		current++;
	}
	current++;//One more because it cuts off the second it SEEs the right card, doesnt go into the while loop to incriment.

	for (int shifting = current; shifting < Hand[0].cardCount; shifting++) {
		HandPassalong[shifting - 1].data[0] = Hand[shifting].data[0];//This is why we need the extra plus one, because the shifting - 1 part
		HandPassalong[shifting - 1].data[1] = Hand[shifting].data[1];
	}
	HandPassalong[Hand[0].cardCount - 1].data[0] = '\0';//The last card that we cant overwrite with the next, because there is no next card so we set the values back to null
	HandPassalong[Hand[0].cardCount - 1].data[1] = '\0';

}


//Returns the new current player value
int updateCurrentPlayer(int currentPlayer, int direction, int playerCount) {
	//printf("%d, %d, %d", currentPlayer, direction, playerCount);//DEBUG REMOVE

	//If its going normal
	if (direction == 1 && currentPlayer == playerCount) {//and we are at the last player
		//printf("Go to zero\n");//DEBUG REMOVE
		return(0);//we go back to the first.
	}
	if (direction == 1 && currentPlayer < playerCount) {//and we are not at the last player
		//printf("Add one\n");//DEBUG REMOVE
		currentPlayer++;
		return(currentPlayer);//we go to the next player.
	}



	//If its going antinormal
	if (direction == -1 && currentPlayer == 0) {//and we are at the first player
		//printf("Go to last\n");//DEBUG REMOVE
		return(playerCount);//go to the last player.
	}
	if (direction == -1 && currentPlayer > 0) {//and we are not at the first player
		//printf("Go previous\n");//DEBUG REMOVE
		currentPlayer--;
		return(currentPlayer);//go back a player
	}
}


//Easier way to draw a card than having the code block over and over again
int drawCard(card deck[DECKLENGTH], card* PlayerDrawEntry, card* PlayerFirstEntry, int currentTopdeck) {
	PlayerDrawEntry->data[0] = deck[currentTopdeck].data[0];
	PlayerDrawEntry->data[1] = deck[currentTopdeck].data[1];
	PlayerFirstEntry->cardCount++;
	currentTopdeck++;
	return(currentTopdeck);
}

void displayOthersCardCount(int playerCount, ai AI[MAXNUMBEROFPLAYERS]) {
	switch (playerCount) {//Check to see how many players there are so we can print the right amount of players hands
	case (0):
		printf("Error in play count?");//Shouldnt be zero players
		printf("Data corruption noticed when hitting: Case Zero in switch statement for printing the number of cards the other players have");
		dataCorruption = 1;
		break;
	case(1)://One AI
		printf("AI One has %d cards", AI[1].Player[0].cardCount);
		break;
	case(2)://Two AI
		printf("AI One has %d cards", AI[1].Player[0].cardCount);
		printf(", AI Two has %d cards", AI[2].Player[0].cardCount);
		break;
	case(3)://Three AI
		printf("AI One has %d cards", AI[1].Player[0].cardCount);
		printf(", AI Two has %d cards", AI[2].Player[0].cardCount);
		printf(", AI Three has %d cards", AI[3].Player[0].cardCount);
		break;
	case(4)://Four AI
		printf("AI One has %d cards", AI[1].Player[0].cardCount);
		printf(", AI Two has %d cards", AI[2].Player[0].cardCount);
		printf(", AI Three has %d cards", AI[3].Player[0].cardCount);
		printf(", AI Four has %d cards", AI[4].Player[0].cardCount);
		break;
	default://Somehow a number that isnt listed
		printf("Data corruption noticed when hitting: Default in Switch statement for printing the number of cards the other players have");
		dataCorruption = 1;
		break;
	}
	printf("\n\n");
}

//Takes the input int and prints it.
void intToPrint(int input, int capitalized) {
	if (capitalized == 0) {
		if (input == 0) {
			printf("zero");
		}else if (input == 1) {
			printf("one");
		}else if (input == 2) {
			printf("two");
		}else if (input == 3) {
			printf("three");
		}else if (input == 4) {
			printf("four");
		}else if (input == 5) {
			printf("five");
		}else if (input == 6) {
			printf("six");
		}else if (input == 7) {
			printf("seven");
		}else if (input == 8) {
			printf("eight");
		}else if (input == 9) {
			printf("nine");
		}
	}
	else {
		if (input == 0) {
			printf("Zero");
		}else if (input == 1) {
			printf("One");
		}else if (input == 2) {
			printf("Two");
		}else if (input == 3) {
			printf("Three");
		}else if (input == 4) {
			printf("Four");
		}else if (input == 5) {
			printf("Five");
		}else if (input == 6) {
			printf("Six");
		}else if (input == 7) {
			printf("Seven");
		}else if (input == 8) {
			printf("Eight");
		}else if (input == 9) {
			printf("Nine");
		}
	}
	
}

//Takes the hand input and counts the # of cards of each colour, returning the colour with the most cards
char cardsOfColourCounter(card Hand[DECKLENGTH]) {
	int red = 0;
	int blue = 0;
	int green = 0;
	int yellow = 0;
	for (int counter = 0; counter < Hand[0].cardCount; counter++) {//Run through each card and count the colours
		if (Hand[counter].data[1] == 'r') {
			red++;
		}
		else if (Hand[counter].data[1] == 'b') {
			blue++;
		}
		else if (Hand[counter].data[1] == 'g') {
			green++;
		}
		else if (Hand[counter].data[1] == 'y') {
			yellow++;
		}
	}
	//Super mega high tech find the highest value code block, then return the one with the most
	if (red > blue && red > green && red > yellow) {
		return('r');
	}
	else if (blue >= red && blue >= green && blue >= yellow) {
		return('b');
	}
	else if (green >= red && green >= blue && green >= yellow) {
		return('g');
	}
	else if (yellow >= red && yellow >= blue && yellow >= green) {
		return('y');
	}
	else {
		return('r');//Just make it red if theres literally no other colours in the deck (Only wilds for example, or the last card)
	}
}


//It's bigger, it's better! Ladies and gentlemen its /too much/ for Mr. Incredible! It's finally ready!
card aiV1(card Hand[DECKLENGTH], card Deck[DECKLENGTH], card cardInPlay) {
	int legalPlay = 0; 
	int currentCard = 0;
	for (currentCard = 0; currentCard <= Hand[0].cardCount - 1; currentCard++) {
		legalPlay = isThisLegal(Hand[currentCard], cardInPlay);
		if (legalPlay == 1) {
			return(Hand[currentCard]);
			break;
		}
	}
	card NotLegal;
	NotLegal.returnLegalCheck = 1;
	return(NotLegal);
}

//Code required in main function for AIv1
//if (currentPlayer == 4) {//AI fours turn
//
//	fprintf(debugDump, "AI Fours's turn, direction %d\n", direction);//DEBUG RECORDING
//
//	if (skipped == 1) {//Did we get skipped?
//		fprintf(debugDump, "AI Four was skipped.\n");//DEBUG RECORDING
//		printf("AI Four got skipped.\n\n");
//		currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);
//		skipped = 0;
//
//	}else if (drawTwod == 1) {//Did we get plus 2?
//		fprintf(debugDump, "AI Four was +2d.\n");//DEBUG RECORDING
//		printf("AI Four got Plus Two'd.\n\n");
//		currentTopdeck = drawCard(deck, &PlayerFour[PlayerFour[0].cardCount], &PlayerFour[0], currentTopdeck);
//		currentTopdeck = drawCard(deck, &PlayerFour[PlayerFour[0].cardCount], &PlayerFour[0], currentTopdeck);
//		currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);
//		drawTwod = 0;
//
//	}else {
//		//if (playerCount >= 4) {//Legacy
//			card AIPlay = aiV1(PlayerFour, deck, cardInPlay);
//			if (AIPlay.returnLegalCheck != 1) {//AI can play a card
//				printf("AI Four played ");
//				detailedPrint(AIPlay);
//				printf(" on a ");
//				detailedPrint(cardInPlay);
//				printf("\n");
//				dataShifter(AIPlay, &cardInPlay);
//				removeCardFromHand(AIPlay, PlayerFour, &PlayerFour);
//				PlayerFour[0].cardCount--;
//
//				if (AIPlay.data[0] == 'S') skipped = 1;		//Did we play a skip	
//				if (AIPlay.data[0] == 'R') direction = direction * -1;//Did we reverse
//				if (AIPlay.data[0] == 'T') drawTwod = 1;//Did we play a draw two
//
//			}
//			else {//AI can NOT play a card and has to draw
//				dataShifter(deck[currentTopdeck], &PlayerFour[PlayerFour[0].cardCount]);//Copy the data from the current top of the deck into the next empty space in the players hand
//				currentTopdeck++;//Keep track of the top of the deck
//				PlayerFour[0].cardCount++;//Keep track of the players hand size
//				printf("AI Four draws and it now has %i cards\n", PlayerFour[0].cardCount);
//			}
//		//}
//		printf("\n");
//
//		if (PlayerFour[0].cardCount == 0) {//End of the game check
//			clearscreen();
//			printf("AI Four has won the game!\n");
//			gameRunning = 0;
//			currentPlayer = -1;
//		}
//		else {
//			currentPlayer = updateCurrentPlayer(currentPlayer, direction, playerCount);//End of normal turn
//			fprintf(debugDump, "Normal turn, played %c%c. Direction: %d, new turn %d\n", AIPlay.data[0], AIPlay.data[1], direction, currentPlayer);//DEBUG RECORDING
//		}
//
//	}
//}//End of ai fours turn





//It can support wilds, supports the new data format and contains more of the AI logic within it than V1.
card aiV1_2(card Hand[DECKLENGTH], card cardInPlay, int currentPlayer, int direction) {
	fprintf(debugDump, "AI %d turn, direction %d\n", currentPlayer, direction);//DEBUG RECORDING
	card AIPlay;
	int legalPlay = 0;
	int currentCard = 0;

	int RegWild = 0;//We have a regular wild flag
	int Four = 0;//We have a wild plus four flag
	int WeHaveWild[2] = {0, 0};//Stores position of last wild of each type, first is Wild, second is +4


	for (currentCard = 0; currentCard <= Hand[0].cardCount - 1; currentCard++) {
		legalPlay = isThisLegal(Hand[currentCard], cardInPlay);
		if (legalPlay == 1) {//We have a legal play
			AIPlay.data[0] = Hand[currentCard].data[0];
			AIPlay.data[1] = Hand[currentCard].data[1];

			break;
		}
		else if (legalPlay == 2) {
			WeHaveWild[0] = 1;
			RegWild = currentCard;
		}
		else if (legalPlay == 3) {
			WeHaveWild[1] = 1;
			Four = currentCard;
		}
	}

	if (WeHaveWild[1] == 1) {//I just copied this because I have no reason at the moment to differentiate between playing a +4 and a regular wild from this end
		AIPlay.data[1] = cardsOfColourCounter(Hand);
	}
	if (WeHaveWild[0] == 1) {
		AIPlay.data[1] = cardsOfColourCounter(Hand);
	}

	if (legalPlay == 1) {
		printf("AI ");//Print to user what AI just played what on what! /I thought I needed laughter but it has to come from me/ /If you want laughter then stick with Cheese/
		intToPrint(currentPlayer, 1);
		printf(" played a ");
		detailedPrint(AIPlay);
		printf(" on a ");
		detailedPrint(cardInPlay);
		printf("\n");
		return(AIPlay);
	}
	else {
		AIPlay.data[0] = 'D';
		return(AIPlay);
	}

}