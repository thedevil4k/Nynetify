#ifndef LANG_H
#define LANG_H

struct Lang {
    // Sidebar
    const char* home;
    const char* search;
    const char* your_library;
    const char* liked_songs;
    const char* create_playlist;
    const char* settings;
    const char* language_btn;
    const char* credits;

    // Home
    const char* browse_categories;
    const char* card_cats[6];
    const char* now_playing_featured;
    const char* feat_desc;

    // Search
    const char* search_tooltip;
    const char* everything;
    const char* songs_filter;
    const char* playlists_filter;
    const char* channels_filter;
    const char* yt_label;
    const char* twitch_label;
    const char* soundcloud_label;
    const char* twitch_live;
    const char* twitch_category;

    // Playlist view
    const char* playlist_name_placeholder;
    const char* playlist_desc_placeholder;
    const char* delete_playlist;
    const char* favorites_desc;
    const char* yt_playlist_by;
    const char* yt_channel;

    // Player
    const char* not_playing;
    const char* view_channel;

    // Show more
    const char* show_more_text;
    const char* show_more_prefix;
    const char* remaining_suffix;

    // Dialogs – titles
    const char* new_playlist_title;
    const char* add_to_playlist_title;
    const char* eq_title;
    const char* settings_title;

    // Dialogs – labels
    const char* name_label;
    const char* comment_label;
    const char* create_btn;
    const char* add_btn;
    const char* favorites_btn;
    const char* ok_btn;
    const char* yes_btn;
    const char* no_btn;

    // Messages
    const char* added_to_playlist;
    const char* cannot_delete_favorites;
    const char* cannot_delete_yt_playlist;
    const char* cannot_delete_channel;
    const char* delete_confirm;
    const char* channel_id_unavailable;

    // Browser messages
    const char* search_failed;
    const char* loading_playlist;
    const char* no_tracks;
    const char* loading_favorites;
    const char* no_favorites;
    const char* loading_yt_playlist;
    const char* no_yt_tracks;
    const char* loading_channel;
    const char* no_channel_content;

    // Channel section headers
    const char* channel_playlists;
    const char* channel_videos;

    // Settings
    const char* load_thumbnails;
    const char* show_status_bar;
    const char* max_buffer_label;
    const char* fetch_size_label;
    const char* batch_label;
    const char* enable_antialiasing;
    const char* enable_debug_mode;
    const char* open_logs_btn;
    const char* search_provider_label;
    const char* provider_invidious;
    const char* provider_ytdlp;

    // EQ
    const char* enable_eq;

    // Download
    const char* download_song;
    const char* downloading;
    const char* download_completed;
    const char* download_path;
    const char* browse;

    // Context menu
    const char* delete_from_playlist;

    // Radio
    const char* radio;
    const char* radio_all_countries;
    const char* radio_add_custom;
    const char* radio_search_online;
    const char* radio_favs;
    const char* radio_no_stations;
    const char* radio_live;
    const char* radio_station_url;
    const char* radio_custom_name;
    const char* radio_custom_url;
    const char* radio_refresh_url;

    // Status bar / window title
    const char* status_prefix;
    const char* window_title;
};

inline Lang LANG_EN = {
    // Sidebar
    "   Home",
    "   Search",
    "YOUR LIBRARY",
    "   Liked Songs",
    "+ Create Playlist",
    "Settings",
    "ES",
    "Credits",
    // Home
    "Browse Categories",
    {"Classic Rock", "Hip Hop", "Synthwave", "Lo-fi Beats", "Top Hits", "Liked Songs"},
    "Now Playing / Featured",
    "Connect, play, and discover millions of tracks directly via Youtube Audio streaming.\n"
    "Create playlists inside the Your Library section, favorite your beloved tracks with the heart button, "
    "and fine-tune your frequencies using our build-in 10-band equalizer.",
    // Search
    "What do you want to listen to?",
    "Everything",
    "Songs",
    "Playlists",
    "Channels",
    "YouTube",
    "Twitch",
    "SoundCloud",
    "LIVE",
    "Category",
    // Playlist view
    "Playlist Name",
    "A custom playlist created in Nynetify.",
    "Delete Playlist",
    "Your personal favorite tracks.",
    "YouTube Playlist by ",
    "YouTube Channel",
    // Player
    "Not Playing",
    "View channel",
    // Show more
    "Show more",
    "Show more (",
    " remaining)",
    // Dialogs – titles
    "New Playlist",
    "Add to Playlist",
    "Equalizer (10 Bands)",
    "Settings",
    // Dialogs – labels
    "Name:",
    "Comment:",
    "CREATE",
    "ADD",
    "FAVORITES",
    "OK",
    "Yes",
    "No",
    // Messages
    "Added to playlist!",
    "You cannot delete the Liked Songs list.",
    "You cannot delete a YouTube playlist from here.",
    "You cannot delete a channel from here.",
    "Are you sure you want to delete this playlist?",
    "Channel ID not available",
    // Browser messages
    "Error: Search failed. Check console for details.",
    "Loading playlist tracks...",
    "No tracks in this playlist.",
    "Loading favorites...",
    "No liked songs found.",
    "Loading YouTube playlist tracks...",
    "No tracks found in this playlist.",
    "Loading channel content...",
    "No content found for this channel.",
    // Channel section headers
    "\xe2\x96\xb6 Playlists",
    "\xe2\x99\xab Videos",
    // Settings
    " Load Thumbnails",
    " Show Status Bar",
    "Max Buffer Size: %d MB",
    "Fetch size: %d results",
    "Batch: +%d per scroll",
    " Enable Antialiasing",
    " Enable Debug Mode",
    " Open Logs",
    " Search Provider",
    "Invidious (Fast, Native)",
    "yt-dlp (Compat, Slow)",
    // EQ
    " ENABLE EQ",
    // Download
    "Download",
    "Downloading %s...",
    "Downloaded: %s",
    "Download path:",
    "Browse",
    // Context menu
    "Delete from Playlist",
    // Radio
    "   Radio",
    "All Countries",
    "Add Custom Station...",
    "Search Online...",
    "Favorites",
    "No stations found",
    "LIVE",
    "Station URL:",
    "Station Name:",
    "Stream URL:",
    "Refresh URL",
    // Status / title
    "Nynetify v1.0",
    "Nynetify"
};

inline Lang LANG_ES = {
    // Sidebar
    "   Inicio",
    "   Buscar",
    "TU BIBLIOTECA",
    "   Canciones que me gustan",
    "+ Crear lista",
    "Ajustes",
    "EN",
    "Creditos",
    // Home
    "Explorar categorias",
    {"Rock Clasico", "Hip Hop", "Synthwave", "Lo-fi Beats", "Exitos", "Me gusta"},
    "Reproduciendo / Destacados",
    "Conecta, reproduce y descubre millones de canciones via Youtube Audio.\n"
    "Crea listas de reproduccion, marca tus canciones favoritas con el corazon "
    "y ajusta el sonido con el ecualizador integrado de 10 bandas.",
    // Search
    "Que quieres escuchar?",
    "Todo",
    "Canciones",
    "Listas",
    "Canales",
    "YouTube",
    "Twitch",
    "SoundCloud",
    "EN VIVO",
    "Categoria",
    // Playlist view
    "Nombre de lista",
    "Una lista creada en Nynetify.",
    "Eliminar lista",
    "Tus canciones favoritas.",
    "Lista de YouTube de ",
    "Canal de YouTube",
    // Player
    "Sin reproduccion",
    "Ver canal",
    // Show more
    "Mostrar mas",
    "Mostrar mas (",
    " restantes)",
    // Dialogs – titles
    "Nueva lista",
    "Anadir a lista",
    "Ecualizador (10 bandas)",
    "Ajustes",
    // Dialogs – labels
    "Nombre:",
    "Comentario:",
    "CREAR",
    "ANADIR",
    "FAVORITOS",
    "OK",
    "Si",
    "No",
    // Messages
    "Anadido a la lista!",
    "No puedes eliminar la lista de canciones que me gustan.",
    "No puedes eliminar una lista de YouTube desde aqui.",
    "No puedes eliminar un canal desde aqui.",
    "Seguro que quieres eliminar esta lista?",
    "ID de canal no disponible",
    // Browser messages
    "Error: Fallo en la busqueda. Revisa la consola.",
    "Cargando canciones de la lista...",
    "No hay canciones en esta lista.",
    "Cargando favoritos...",
    "No hay canciones favoritas.",
    "Cargando lista de YouTube...",
    "No hay canciones en esta lista de YouTube.",
    "Cargando contenido del canal...",
    "No hay contenido en este canal.",
    // Channel section headers
    "\xe2\x96\xb6 Listas",
    "\xe2\x99\xab Videos",
    // Settings
    " Cargar miniaturas",
    " Mostrar barra de estado",
    "Tamano maximo de buffer: %d MB",
    "Tamano de busqueda: %d resultados",
    "Lote: +%d por scroll",
    " Activar Antialiasing",
    " Modo Debug",
    " Abrir logs",
    " Proveedor de Busqueda",
    "Invidious (Rapido, Nativo)",
    "yt-dlp (Compatible, Lento)",
    // EQ
    " ACTIVAR EQ",
    // Download
    "Descargar",
    "Descargando %s...",
    "Descargado: %s",
    "Ruta de descarga:",
    "Examinar",
    // Context menu
    "Eliminar de la lista",
    // Radio
    "   Radio",
    "Todos los paises",
    "Anadir emisora personalizada...",
    "Buscar online...",
    "Favoritas",
    "No se encontraron emisoras",
    "EN VIVO",
    "URL de la emisora:",
    "Nombre de la emisora:",
    "URL del stream:",
    "Refrescar URL",
    // Status / title
    "Nynetify v1.0",
    "Nynetify"
};

inline Lang* lang = &LANG_EN;

#endif
