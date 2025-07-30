# Requisiti





# Scenario 
### Inizializzazione server
1. server si accende
2. inizializzo server NTP
    1. ???? errore --> *eccezione terminazione*
1. lettura DB
    1. creo DB se non esiste
        1. creazione non va a buon fine --> *eccezione terminazione* 
2. lettura file di configurazione
    1. file non esiste --> *eccezione terminazione*
    2. lettura errata sintassi file --> *eccezione terminazione*
### Connessione schede
1. Aspetto N connessione con N definito nel file di configurazione
    1. se non ricevo N connessioni scatta un timeout dopo 30 sec --> *eccezione terminazione*
2. Calcolo parametri per la regressione
3. Creazione N thread e consumer
    1. Memoria non disponibile --> *eccezione, riprovo e se non riesco dopo qualche tentativo termino*
4. Invio OK a ogni scheda per iniziare a ricevere i pacchetti
### Acquisizione dati
1. ogni thread legge dal socket la probe request e lo inserisce nella coda condivisa
    1. se il thread non riesce a leggere dal socket --> *riprovo a riconnettermi alla schedina e in 30 secondi se non riesco eccezione termino* 
2. consumer thread prende i pacchetti e li accoppia per hash
3. consumer calcola la posizione 
4. inserisco posizione nel db
5. torno al punto 1.

