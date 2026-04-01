/**
 * 3DS CloudSaver - Multi-Language String Tables
 * ──────────────────────────────────────────────
 * All user-facing strings, organised by StringKey enum.
 */

#include "lang.h"

static Language current_lang = LANG_EN;

/*═══════════════════════════════════════════════════════════════*
 *  String tables – one array per language
 *═══════════════════════════════════════════════════════════════*/

/* ──────────── English ──────────── */
static const char *strings_en[STR_COUNT] = {
    /* General */
    [STR_APP_TITLE]     = "3DS CloudSaver",
    [STR_OK]            = "OK",
    [STR_CANCEL]        = "Cancel",
    [STR_BACK]          = "Back",
    [STR_YES]           = "Yes",
    [STR_NO]            = "No",
    [STR_ERROR]         = "Error",
    [STR_SUCCESS]       = "Success!",
    [STR_LOADING]       = "Loading...",
    [STR_PLEASE_WAIT]   = "Please wait...",

    /* Setup */
    [STR_WELCOME]           = "Welcome to 3DS CloudSaver!",
    [STR_ENTER_DEVICE_NAME] = "Please name this device",
    [STR_DEVICE_NAME_HINT]  = "e.g. My3DS, BlueStar...",

    /* Main */
    [STR_NO_GAMES_FOUND]  = "No games found.",
    [STR_INSTALLED_GAMES] = "Installed Games",
    [STR_SELECT_GAME]     = "Select a game",
    [STR_NO_SAVES]        = "No save files found.",
    [STR_SAVES_FOR]       = "Saves for:",

    /* Save details */
    [STR_SAVE_CREATED]      = "Created:",
    [STR_SAVE_DEVICE]       = "Device:",
    [STR_SAVE_GAME]         = "Game:",
    [STR_SAVE_DESCRIPTION]  = "Description:",
    [STR_SAVE_SIZE]         = "Size:",
    [STR_SAVE_REGION]       = "Region:",
    [STR_ADD_DESCRIPTION]   = "Add a description",
    [STR_EDIT_DESCRIPTION]  = "Edit description",

    /* Sync */
    [STR_SYNC_ALL]          = "Sync All",
    [STR_SYNC_ALL_DESC]     = "Upload all saves to the cloud",
    [STR_SYNC_IN_PROGRESS]  = "Syncing...",
    [STR_SYNC_COMPLETE]     = "Sync complete!",
    [STR_SYNC_FAILED]       = "Sync failed",
    [STR_ENTER_COMMIT_MSG]  = "Enter commit message",
    [STR_COMMIT_MSG_HINT]   = "e.g. Current savestates",
    [STR_UPLOAD_SAVE]       = "Upload Save",
    [STR_DOWNLOAD_SAVE]     = "Download Save",

    /* Navigation */
    [STR_NAV_GAMES]    = "Games",
    [STR_NAV_SYNC]     = "Sync All",
    [STR_NAV_SETTINGS] = "Settings",

    /* Settings */
    [STR_SETTINGS]              = "Settings",
    [STR_SETTINGS_SERVER]       = "Server",
    [STR_SETTINGS_LANGUAGE]     = "Language",
    [STR_SETTINGS_DEVICE]       = "Device Name",
    [STR_SETTINGS_AUTO_CONNECT] = "Auto Connect",
    [STR_SETTINGS_AUTO_SYNC]    = "Auto Sync",
    [STR_SERVER_HOST]           = "Host:",
    [STR_SERVER_PORT]           = "Port:",
    [STR_SERVER_USER]           = "Username:",
    [STR_SERVER_PASS]           = "Password:",
    [STR_SERVER_PATH]           = "Remote Path:",
    [STR_SERVER_TYPE]           = "Protocol:",
    [STR_SERVER_TYPE_SFTP]      = "SFTP",
    [STR_SERVER_TYPE_SMB]       = "SMB",
    [STR_SERVER_TYPE_NONE]      = "None",
    [STR_SERVER_CONNECT]        = "Connect",
    [STR_SERVER_DISCONNECT]     = "Disconnect",
    [STR_SERVER_TEST]           = "Test Connection",
    [STR_SERVER_STATUS_OK]      = "Connected",
    [STR_SERVER_STATUS_FAIL]    = "Not Connected",
    [STR_SERVER_NOT_CONFIGURED] = "Server not configured",

    /* Connection */
    [STR_CONNECTING]        = "Connecting...",
    [STR_CONNECTED]         = "Connected",
    [STR_DISCONNECTED]      = "Disconnected",
    [STR_CONNECTION_FAILED]  = "Connection failed",
    [STR_CONNECTION_SUCCESS] = "Connection successful!",

    /* Regions */
    [STR_REGION_EUR]     = "EUR",
    [STR_REGION_USA]     = "USA",
    [STR_REGION_JPN]     = "JPN",
    [STR_REGION_KOR]     = "KOR",
    [STR_REGION_UNKNOWN] = "ALL",

    /* Language names */
    [STR_LANG_EN] = "English",
    [STR_LANG_DE] = "Deutsch",
    [STR_LANG_FR] = "Français",
    [STR_LANG_ES] = "Español",
    [STR_LANG_IT] = "Italiano",
    [STR_LANG_JA] = "日本語",
};

/* ──────────── Deutsch (German) ──────────── */
static const char *strings_de[STR_COUNT] = {
    [STR_APP_TITLE]     = "3DS CloudSaver",
    [STR_OK]            = "OK",
    [STR_CANCEL]        = "Abbrechen",
    [STR_BACK]          = "Zurück",
    [STR_YES]           = "Ja",
    [STR_NO]            = "Nein",
    [STR_ERROR]         = "Fehler",
    [STR_SUCCESS]       = "Erfolgreich!",
    [STR_LOADING]       = "Lade...",
    [STR_PLEASE_WAIT]   = "Bitte warten...",

    [STR_WELCOME]           = "Willkommen bei 3DS CloudSaver!",
    [STR_ENTER_DEVICE_NAME] = "Bitte gib diesem Gerät einen Namen",
    [STR_DEVICE_NAME_HINT]  = "z.B. Mein3DS, BlauStern...",

    [STR_NO_GAMES_FOUND]  = "Keine Spiele gefunden.",
    [STR_INSTALLED_GAMES] = "Installierte Spiele",
    [STR_SELECT_GAME]     = "Spiel auswählen",
    [STR_NO_SAVES]        = "Keine Spielstände gefunden.",
    [STR_SAVES_FOR]       = "Spielstände für:",

    [STR_SAVE_CREATED]      = "Erstellt:",
    [STR_SAVE_DEVICE]       = "Gerät:",
    [STR_SAVE_GAME]         = "Spiel:",
    [STR_SAVE_DESCRIPTION]  = "Beschreibung:",
    [STR_SAVE_SIZE]         = "Größe:",
    [STR_SAVE_REGION]       = "Region:",
    [STR_ADD_DESCRIPTION]   = "Beschreibung hinzufügen",
    [STR_EDIT_DESCRIPTION]  = "Beschreibung bearbeiten",

    [STR_SYNC_ALL]          = "Alle synchronisieren",
    [STR_SYNC_ALL_DESC]     = "Alle Spielstände in die Cloud hochladen",
    [STR_SYNC_IN_PROGRESS]  = "Synchronisiere...",
    [STR_SYNC_COMPLETE]     = "Synchronisierung abgeschlossen!",
    [STR_SYNC_FAILED]       = "Synchronisierung fehlgeschlagen",
    [STR_ENTER_COMMIT_MSG]  = "Commit-Nachricht eingeben",
    [STR_COMMIT_MSG_HINT]   = "z.B. Aktuelle Savestates",
    [STR_UPLOAD_SAVE]       = "Spielstand hochladen",
    [STR_DOWNLOAD_SAVE]     = "Spielstand herunterladen",

    [STR_NAV_GAMES]    = "Spiele",
    [STR_NAV_SYNC]     = "Sync",
    [STR_NAV_SETTINGS] = "Einstellungen",

    [STR_SETTINGS]              = "Einstellungen",
    [STR_SETTINGS_SERVER]       = "Server",
    [STR_SETTINGS_LANGUAGE]     = "Sprache",
    [STR_SETTINGS_DEVICE]       = "Gerätename",
    [STR_SETTINGS_AUTO_CONNECT] = "Auto-Verbindung",
    [STR_SETTINGS_AUTO_SYNC]    = "Auto-Sync",
    [STR_SERVER_HOST]           = "Host:",
    [STR_SERVER_PORT]           = "Port:",
    [STR_SERVER_USER]           = "Benutzer:",
    [STR_SERVER_PASS]           = "Passwort:",
    [STR_SERVER_PATH]           = "Remote-Pfad:",
    [STR_SERVER_TYPE]           = "Protokoll:",
    [STR_SERVER_TYPE_SFTP]      = "SFTP",
    [STR_SERVER_TYPE_SMB]       = "SMB",
    [STR_SERVER_TYPE_NONE]      = "Keins",
    [STR_SERVER_CONNECT]        = "Verbinden",
    [STR_SERVER_DISCONNECT]     = "Trennen",
    [STR_SERVER_TEST]           = "Verbindung testen",
    [STR_SERVER_STATUS_OK]      = "Verbunden",
    [STR_SERVER_STATUS_FAIL]    = "Nicht verbunden",
    [STR_SERVER_NOT_CONFIGURED] = "Server nicht konfiguriert",

    [STR_CONNECTING]         = "Verbinde...",
    [STR_CONNECTED]          = "Verbunden",
    [STR_DISCONNECTED]       = "Getrennt",
    [STR_CONNECTION_FAILED]  = "Verbindung fehlgeschlagen",
    [STR_CONNECTION_SUCCESS] = "Verbindung erfolgreich!",

    [STR_REGION_EUR]     = "EUR",
    [STR_REGION_USA]     = "USA",
    [STR_REGION_JPN]     = "JPN",
    [STR_REGION_KOR]     = "KOR",
    [STR_REGION_UNKNOWN] = "ALL",

    [STR_LANG_EN] = "English",
    [STR_LANG_DE] = "Deutsch",
    [STR_LANG_FR] = "Français",
    [STR_LANG_ES] = "Español",
    [STR_LANG_IT] = "Italiano",
    [STR_LANG_JA] = "日本語",
};

/* ──────────── Français (French) ──────────── */
static const char *strings_fr[STR_COUNT] = {
    [STR_APP_TITLE]     = "3DS CloudSaver",
    [STR_OK]            = "OK",
    [STR_CANCEL]        = "Annuler",
    [STR_BACK]          = "Retour",
    [STR_YES]           = "Oui",
    [STR_NO]            = "Non",
    [STR_ERROR]         = "Erreur",
    [STR_SUCCESS]       = "Succès !",
    [STR_LOADING]       = "Chargement...",
    [STR_PLEASE_WAIT]   = "Veuillez patienter...",

    [STR_WELCOME]           = "Bienvenue dans 3DS CloudSaver !",
    [STR_ENTER_DEVICE_NAME] = "Veuillez nommer cet appareil",
    [STR_DEVICE_NAME_HINT]  = "ex. Ma3DS, EtoileRouge...",

    [STR_NO_GAMES_FOUND]  = "Aucun jeu trouvé.",
    [STR_INSTALLED_GAMES] = "Jeux installés",
    [STR_SELECT_GAME]     = "Sélectionner un jeu",
    [STR_NO_SAVES]        = "Aucune sauvegarde trouvée.",
    [STR_SAVES_FOR]       = "Sauvegardes pour :",

    [STR_SAVE_CREATED]      = "Créé le :",
    [STR_SAVE_DEVICE]       = "Appareil :",
    [STR_SAVE_GAME]         = "Jeu :",
    [STR_SAVE_DESCRIPTION]  = "Description :",
    [STR_SAVE_SIZE]         = "Taille :",
    [STR_SAVE_REGION]       = "Région :",
    [STR_ADD_DESCRIPTION]   = "Ajouter une description",
    [STR_EDIT_DESCRIPTION]  = "Modifier la description",

    [STR_SYNC_ALL]          = "Tout synchroniser",
    [STR_SYNC_ALL_DESC]     = "Envoyer toutes les sauvegardes",
    [STR_SYNC_IN_PROGRESS]  = "Synchronisation...",
    [STR_SYNC_COMPLETE]     = "Synchronisation terminée !",
    [STR_SYNC_FAILED]       = "Échec de la synchronisation",
    [STR_ENTER_COMMIT_MSG]  = "Entrer un message",
    [STR_COMMIT_MSG_HINT]   = "ex. Sauvegardes actuelles",
    [STR_UPLOAD_SAVE]       = "Envoyer la sauvegarde",
    [STR_DOWNLOAD_SAVE]     = "Télécharger la sauvegarde",

    [STR_NAV_GAMES]    = "Jeux",
    [STR_NAV_SYNC]     = "Sync",
    [STR_NAV_SETTINGS] = "Paramètres",

    [STR_SETTINGS]              = "Paramètres",
    [STR_SETTINGS_SERVER]       = "Serveur",
    [STR_SETTINGS_LANGUAGE]     = "Langue",
    [STR_SETTINGS_DEVICE]       = "Nom de l'appareil",
    [STR_SETTINGS_AUTO_CONNECT] = "Connexion auto",
    [STR_SETTINGS_AUTO_SYNC]    = "Sync auto",
    [STR_SERVER_HOST]           = "Hôte :",
    [STR_SERVER_PORT]           = "Port :",
    [STR_SERVER_USER]           = "Utilisateur :",
    [STR_SERVER_PASS]           = "Mot de passe :",
    [STR_SERVER_PATH]           = "Chemin distant :",
    [STR_SERVER_TYPE]           = "Protocole :",
    [STR_SERVER_TYPE_SFTP]      = "SFTP",
    [STR_SERVER_TYPE_SMB]       = "SMB",
    [STR_SERVER_TYPE_NONE]      = "Aucun",
    [STR_SERVER_CONNECT]        = "Connecter",
    [STR_SERVER_DISCONNECT]     = "Déconnecter",
    [STR_SERVER_TEST]           = "Tester la connexion",
    [STR_SERVER_STATUS_OK]      = "Connecté",
    [STR_SERVER_STATUS_FAIL]    = "Non connecté",
    [STR_SERVER_NOT_CONFIGURED] = "Serveur non configuré",

    [STR_CONNECTING]         = "Connexion en cours...",
    [STR_CONNECTED]          = "Connecté",
    [STR_DISCONNECTED]       = "Déconnecté",
    [STR_CONNECTION_FAILED]  = "Connexion échouée",
    [STR_CONNECTION_SUCCESS] = "Connexion réussie !",

    [STR_REGION_EUR]     = "EUR",
    [STR_REGION_USA]     = "USA",
    [STR_REGION_JPN]     = "JPN",
    [STR_REGION_KOR]     = "KOR",
    [STR_REGION_UNKNOWN] = "ALL",

    [STR_LANG_EN] = "English",
    [STR_LANG_DE] = "Deutsch",
    [STR_LANG_FR] = "Français",
    [STR_LANG_ES] = "Español",
    [STR_LANG_IT] = "Italiano",
    [STR_LANG_JA] = "日本語",
};

/* ──────────── Español (Spanish) ──────────── */
static const char *strings_es[STR_COUNT] = {
    [STR_APP_TITLE]     = "3DS CloudSaver",
    [STR_OK]            = "OK",
    [STR_CANCEL]        = "Cancelar",
    [STR_BACK]          = "Volver",
    [STR_YES]           = "Sí",
    [STR_NO]            = "No",
    [STR_ERROR]         = "Error",
    [STR_SUCCESS]       = "¡Éxito!",
    [STR_LOADING]       = "Cargando...",
    [STR_PLEASE_WAIT]   = "Por favor espera...",

    [STR_WELCOME]           = "¡Bienvenido a 3DS CloudSaver!",
    [STR_ENTER_DEVICE_NAME] = "Nombra este dispositivo",
    [STR_DEVICE_NAME_HINT]  = "ej. Mi3DS, EstrellAzul...",

    [STR_NO_GAMES_FOUND]  = "No se encontraron juegos.",
    [STR_INSTALLED_GAMES] = "Juegos instalados",
    [STR_SELECT_GAME]     = "Seleccionar juego",
    [STR_NO_SAVES]        = "No se encontraron partidas.",
    [STR_SAVES_FOR]       = "Guardados de:",

    [STR_SAVE_CREATED]     = "Creado:",
    [STR_SAVE_DEVICE]      = "Dispositivo:",
    [STR_SAVE_GAME]        = "Juego:",
    [STR_SAVE_DESCRIPTION] = "Descripción:",
    [STR_SAVE_SIZE]        = "Tamaño:",
    [STR_SAVE_REGION]      = "Región:",
    [STR_ADD_DESCRIPTION]  = "Añadir descripción",
    [STR_EDIT_DESCRIPTION] = "Editar descripción",

    [STR_SYNC_ALL]          = "Sincronizar todo",
    [STR_SYNC_ALL_DESC]     = "Subir todas las partidas a la nube",
    [STR_SYNC_IN_PROGRESS]  = "Sincronizando...",
    [STR_SYNC_COMPLETE]     = "¡Sincronización completa!",
    [STR_SYNC_FAILED]       = "Sincronización fallida",
    [STR_ENTER_COMMIT_MSG]  = "Ingresa un mensaje",
    [STR_COMMIT_MSG_HINT]   = "ej. Partidas actuales",
    [STR_UPLOAD_SAVE]       = "Subir partida",
    [STR_DOWNLOAD_SAVE]     = "Descargar partida",

    [STR_NAV_GAMES]    = "Juegos",
    [STR_NAV_SYNC]     = "Sync",
    [STR_NAV_SETTINGS] = "Ajustes",

    [STR_SETTINGS]              = "Ajustes",
    [STR_SETTINGS_SERVER]       = "Servidor",
    [STR_SETTINGS_LANGUAGE]     = "Idioma",
    [STR_SETTINGS_DEVICE]       = "Nombre del dispositivo",
    [STR_SETTINGS_AUTO_CONNECT] = "Conexión automática",
    [STR_SETTINGS_AUTO_SYNC]    = "Sync automático",
    [STR_SERVER_HOST]           = "Host:",
    [STR_SERVER_PORT]           = "Puerto:",
    [STR_SERVER_USER]           = "Usuario:",
    [STR_SERVER_PASS]           = "Contraseña:",
    [STR_SERVER_PATH]           = "Ruta remota:",
    [STR_SERVER_TYPE]           = "Protocolo:",
    [STR_SERVER_TYPE_SFTP]      = "SFTP",
    [STR_SERVER_TYPE_SMB]       = "SMB",
    [STR_SERVER_TYPE_NONE]      = "Ninguno",
    [STR_SERVER_CONNECT]        = "Conectar",
    [STR_SERVER_DISCONNECT]     = "Desconectar",
    [STR_SERVER_TEST]           = "Probar conexión",
    [STR_SERVER_STATUS_OK]      = "Conectado",
    [STR_SERVER_STATUS_FAIL]    = "No conectado",
    [STR_SERVER_NOT_CONFIGURED] = "Servidor no configurado",

    [STR_CONNECTING]         = "Conectando...",
    [STR_CONNECTED]          = "Conectado",
    [STR_DISCONNECTED]       = "Desconectado",
    [STR_CONNECTION_FAILED]  = "Conexión fallida",
    [STR_CONNECTION_SUCCESS] = "¡Conexión exitosa!",

    [STR_REGION_EUR]     = "EUR",
    [STR_REGION_USA]     = "USA",
    [STR_REGION_JPN]     = "JPN",
    [STR_REGION_KOR]     = "KOR",
    [STR_REGION_UNKNOWN] = "ALL",

    [STR_LANG_EN] = "English",
    [STR_LANG_DE] = "Deutsch",
    [STR_LANG_FR] = "Français",
    [STR_LANG_ES] = "Español",
    [STR_LANG_IT] = "Italiano",
    [STR_LANG_JA] = "日本語",
};

/* ──────────── Italiano (Italian) ──────────── */
static const char *strings_it[STR_COUNT] = {
    [STR_APP_TITLE]     = "3DS CloudSaver",
    [STR_OK]            = "OK",
    [STR_CANCEL]        = "Annulla",
    [STR_BACK]          = "Indietro",
    [STR_YES]           = "Sì",
    [STR_NO]            = "No",
    [STR_ERROR]         = "Errore",
    [STR_SUCCESS]       = "Successo!",
    [STR_LOADING]       = "Caricamento...",
    [STR_PLEASE_WAIT]   = "Attendere prego...",

    [STR_WELCOME]           = "Benvenuto in 3DS CloudSaver!",
    [STR_ENTER_DEVICE_NAME] = "Assegna un nome a questo dispositivo",
    [STR_DEVICE_NAME_HINT]  = "es. Mio3DS, StellaBlu...",

    [STR_NO_GAMES_FOUND]  = "Nessun gioco trovato.",
    [STR_INSTALLED_GAMES] = "Giochi installati",
    [STR_SELECT_GAME]     = "Seleziona un gioco",
    [STR_NO_SAVES]        = "Nessun salvataggio trovato.",
    [STR_SAVES_FOR]       = "Salvataggi per:",

    [STR_SAVE_CREATED]     = "Creato:",
    [STR_SAVE_DEVICE]      = "Dispositivo:",
    [STR_SAVE_GAME]        = "Gioco:",
    [STR_SAVE_DESCRIPTION] = "Descrizione:",
    [STR_SAVE_SIZE]        = "Dimensione:",
    [STR_SAVE_REGION]      = "Regione:",
    [STR_ADD_DESCRIPTION]  = "Aggiungi descrizione",
    [STR_EDIT_DESCRIPTION] = "Modifica descrizione",

    [STR_SYNC_ALL]          = "Sincronizza tutto",
    [STR_SYNC_ALL_DESC]     = "Carica tutti i salvataggi nel cloud",
    [STR_SYNC_IN_PROGRESS]  = "Sincronizzazione...",
    [STR_SYNC_COMPLETE]     = "Sincronizzazione completata!",
    [STR_SYNC_FAILED]       = "Sincronizzazione fallita",
    [STR_ENTER_COMMIT_MSG]  = "Inserisci un messaggio",
    [STR_COMMIT_MSG_HINT]   = "es. Salvataggi attuali",
    [STR_UPLOAD_SAVE]       = "Carica salvataggio",
    [STR_DOWNLOAD_SAVE]     = "Scarica salvataggio",

    [STR_NAV_GAMES]    = "Giochi",
    [STR_NAV_SYNC]     = "Sync",
    [STR_NAV_SETTINGS] = "Impostazioni",

    [STR_SETTINGS]              = "Impostazioni",
    [STR_SETTINGS_SERVER]       = "Server",
    [STR_SETTINGS_LANGUAGE]     = "Lingua",
    [STR_SETTINGS_DEVICE]       = "Nome dispositivo",
    [STR_SETTINGS_AUTO_CONNECT] = "Connessione auto",
    [STR_SETTINGS_AUTO_SYNC]    = "Sync auto",
    [STR_SERVER_HOST]           = "Host:",
    [STR_SERVER_PORT]           = "Porta:",
    [STR_SERVER_USER]           = "Utente:",
    [STR_SERVER_PASS]           = "Password:",
    [STR_SERVER_PATH]           = "Percorso remoto:",
    [STR_SERVER_TYPE]           = "Protocollo:",
    [STR_SERVER_TYPE_SFTP]      = "SFTP",
    [STR_SERVER_TYPE_SMB]       = "SMB",
    [STR_SERVER_TYPE_NONE]      = "Nessuno",
    [STR_SERVER_CONNECT]        = "Connetti",
    [STR_SERVER_DISCONNECT]     = "Disconnetti",
    [STR_SERVER_TEST]           = "Test connessione",
    [STR_SERVER_STATUS_OK]      = "Connesso",
    [STR_SERVER_STATUS_FAIL]    = "Non connesso",
    [STR_SERVER_NOT_CONFIGURED] = "Server non configurato",

    [STR_CONNECTING]         = "Connessione in corso...",
    [STR_CONNECTED]          = "Connesso",
    [STR_DISCONNECTED]       = "Disconnesso",
    [STR_CONNECTION_FAILED]  = "Connessione fallita",
    [STR_CONNECTION_SUCCESS] = "Connessione riuscita!",

    [STR_REGION_EUR]     = "EUR",
    [STR_REGION_USA]     = "USA",
    [STR_REGION_JPN]     = "JPN",
    [STR_REGION_KOR]     = "KOR",
    [STR_REGION_UNKNOWN] = "ALL",

    [STR_LANG_EN] = "English",
    [STR_LANG_DE] = "Deutsch",
    [STR_LANG_FR] = "Français",
    [STR_LANG_ES] = "Español",
    [STR_LANG_IT] = "Italiano",
    [STR_LANG_JA] = "日本語",
};

/* ──────────── 日本語 (Japanese) ──────────── */
static const char *strings_ja[STR_COUNT] = {
    [STR_APP_TITLE]     = "3DS CloudSaver",
    [STR_OK]            = "OK",
    [STR_CANCEL]        = "キャンセル",
    [STR_BACK]          = "戻る",
    [STR_YES]           = "はい",
    [STR_NO]            = "いいえ",
    [STR_ERROR]         = "エラー",
    [STR_SUCCESS]       = "成功！",
    [STR_LOADING]       = "読み込み中...",
    [STR_PLEASE_WAIT]   = "お待ちください...",

    [STR_WELCOME]           = "3DS CloudSaverへようこそ！",
    [STR_ENTER_DEVICE_NAME] = "デバイス名を入力してください",
    [STR_DEVICE_NAME_HINT]  = "例: My3DS",

    [STR_NO_GAMES_FOUND]  = "ゲームが見つかりません。",
    [STR_INSTALLED_GAMES] = "インストール済みゲーム",
    [STR_SELECT_GAME]     = "ゲームを選択",
    [STR_NO_SAVES]        = "セーブデータがありません。",
    [STR_SAVES_FOR]       = "セーブデータ:",

    [STR_SAVE_CREATED]     = "作成日:",
    [STR_SAVE_DEVICE]      = "デバイス:",
    [STR_SAVE_GAME]        = "ゲーム:",
    [STR_SAVE_DESCRIPTION] = "説明:",
    [STR_SAVE_SIZE]        = "サイズ:",
    [STR_SAVE_REGION]      = "リージョン:",
    [STR_ADD_DESCRIPTION]  = "説明を追加",
    [STR_EDIT_DESCRIPTION] = "説明を編集",

    [STR_SYNC_ALL]          = "すべて同期",
    [STR_SYNC_ALL_DESC]     = "すべてのセーブをクラウドにアップロード",
    [STR_SYNC_IN_PROGRESS]  = "同期中...",
    [STR_SYNC_COMPLETE]     = "同期完了！",
    [STR_SYNC_FAILED]       = "同期失敗",
    [STR_ENTER_COMMIT_MSG]  = "メッセージを入力",
    [STR_COMMIT_MSG_HINT]   = "例: 現在のセーブデータ",
    [STR_UPLOAD_SAVE]       = "アップロード",
    [STR_DOWNLOAD_SAVE]     = "ダウンロード",

    [STR_NAV_GAMES]    = "ゲーム",
    [STR_NAV_SYNC]     = "同期",
    [STR_NAV_SETTINGS] = "設定",

    [STR_SETTINGS]              = "設定",
    [STR_SETTINGS_SERVER]       = "サーバー",
    [STR_SETTINGS_LANGUAGE]     = "言語",
    [STR_SETTINGS_DEVICE]       = "デバイス名",
    [STR_SETTINGS_AUTO_CONNECT] = "自動接続",
    [STR_SETTINGS_AUTO_SYNC]    = "自動同期",
    [STR_SERVER_HOST]           = "ホスト:",
    [STR_SERVER_PORT]           = "ポート:",
    [STR_SERVER_USER]           = "ユーザー名:",
    [STR_SERVER_PASS]           = "パスワード:",
    [STR_SERVER_PATH]           = "リモートパス:",
    [STR_SERVER_TYPE]           = "プロトコル:",
    [STR_SERVER_TYPE_SFTP]      = "SFTP",
    [STR_SERVER_TYPE_SMB]       = "SMB",
    [STR_SERVER_TYPE_NONE]      = "なし",
    [STR_SERVER_CONNECT]        = "接続",
    [STR_SERVER_DISCONNECT]     = "切断",
    [STR_SERVER_TEST]           = "接続テスト",
    [STR_SERVER_STATUS_OK]      = "接続済み",
    [STR_SERVER_STATUS_FAIL]    = "未接続",
    [STR_SERVER_NOT_CONFIGURED] = "サーバー未設定",

    [STR_CONNECTING]         = "接続中...",
    [STR_CONNECTED]          = "接続済み",
    [STR_DISCONNECTED]       = "切断",
    [STR_CONNECTION_FAILED]  = "接続失敗",
    [STR_CONNECTION_SUCCESS] = "接続成功！",

    [STR_REGION_EUR]     = "EUR",
    [STR_REGION_USA]     = "USA",
    [STR_REGION_JPN]     = "JPN",
    [STR_REGION_KOR]     = "KOR",
    [STR_REGION_UNKNOWN] = "ALL",

    [STR_LANG_EN] = "English",
    [STR_LANG_DE] = "Deutsch",
    [STR_LANG_FR] = "Français",
    [STR_LANG_ES] = "Español",
    [STR_LANG_IT] = "Italiano",
    [STR_LANG_JA] = "日本語",
};

/*═══════════════════════════════════════════════════════════════*
 *  Table of tables
 *═══════════════════════════════════════════════════════════════*/
static const char **string_tables[LANG_COUNT] = {
    [LANG_EN] = strings_en,
    [LANG_DE] = strings_de,
    [LANG_FR] = strings_fr,
    [LANG_ES] = strings_es,
    [LANG_IT] = strings_it,
    [LANG_JA] = strings_ja,
};

/*═══════════════════════════════════════════════════════════════*
 *  API
 *═══════════════════════════════════════════════════════════════*/
void lang_init(Language lang)
{
    current_lang = (lang < LANG_COUNT) ? lang : LANG_EN;
}

void lang_set(Language lang)
{
    current_lang = (lang < LANG_COUNT) ? lang : LANG_EN;
}

Language lang_get(void)
{
    return current_lang;
}

const char *lang_str(StringKey key)
{
    if (key >= STR_COUNT) return "???";

    const char *s = string_tables[current_lang][key];
    if (s) return s;

    /* Fallback to English */
    s = strings_en[key];
    return s ? s : "???";
}

const char *lang_name(Language lang)
{
    switch (lang) {
    case LANG_EN: return strings_en[STR_LANG_EN];
    case LANG_DE: return strings_en[STR_LANG_DE];
    case LANG_FR: return strings_en[STR_LANG_FR];
    case LANG_ES: return strings_en[STR_LANG_ES];
    case LANG_IT: return strings_en[STR_LANG_IT];
    case LANG_JA: return strings_en[STR_LANG_JA];
    default:      return "???";
    }
}

Language lang_next(Language current)
{
    return (Language)(((int)current + 1) % LANG_COUNT);
}
