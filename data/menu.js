document.addEventListener('DOMContentLoaded', () => {
    const nav = document.querySelector('nav[data-shared-menu="true"]');
    if (!nav) return;

    const links = [
        { href: '/', label: 'Tableau de bord' },
        { href: '/stats.html', label: 'Tendances' },
        { href: '/longterm.html', label: 'Historique 24h' },
        { href: '/files.html', label: 'Fichiers' },
        { href: '/logs', label: 'Logs' },
        { href: '/ota.html', label: 'Système' }
    ];

    const active_href = nav.dataset.active || '/';
    nav.innerHTML = links
        .map((link) => {
            const active_class = link.href === active_href ? 'active' : '';
            return `<a href="${link.href}" class="${active_class}">${link.label}</a>`;
        })
        .join('');
});
