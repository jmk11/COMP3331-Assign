server_sources = ['blockedusers.c', 'blockedusers.h', 'clients.c', 'clients.h', 'commands.c', 'commands.h', 
    'message.c', 'message.h', 'server.c', 'server.h', 'serverClients.c', 'serverClients.h', 'user.c', 'user.h']
executable('server', server_sources,
            link_with: common_lib_static,
            install: true,
            include_directories: '..',
            dependencies: [threads])

# dependencies: [openssl],
