document.addEventListener('DOMContentLoaded', () => {
    const nav = document.querySelector('nav[data-shared-menu="true"]');
    if (!nav) return;

    const links = [
        { href: '/', label: 'Tableau de bord' },
        { href: '/stats.html', label: 'Statistiques' },
        { href: '/longterm.html', label: 'Historique' },
        { href: '/system.html', label: 'Système' }
    ];

    const active_href = nav.dataset.active || '/';

    const render = () => {
        nav.innerHTML = links
            .map((link) => {
                const active_class = link.href === active_href ? 'active' : '';
                return `<a href="${link.href}" class="${active_class}">${link.label}</a>`;
            })
            .join('');
    };

    render();

    // Entrée vers le service d'analyse avancée, ajoutée seulement s'il est
    // joignable (détecté sur le réseau, ou adresse saisie dans la page Système).
    //
    // Le lien s'ouvre dans le MÊME onglet : l'utilisateur passe d'un service à
    // l'autre comme entre deux pages, sans accumuler d'onglets. Le service
    // d'analyse propose en retour un lien « Retour à MeteoHub ».
    //
    // Le libellé reprend le nom que le service ANNONCE : il est reconnu à sa
    // capacité, pas à son nom, donc il peut s'appeler comme son propriétaire le
    // souhaite et le menu l'affichera tel quel.
    fetch('/api/analytics')
        .then((response) => (response.ok ? response.json() : null))
        .then((info) => {
            if (!info || !info.available || !info.effective_url) return;
            const detected_name = info.detected && info.detected.name;
            const entry = {
                href: info.effective_url,
                label: detected_name || 'Analyses avancées'
            };
            // Inséré juste avant « Système », qui doit rester la dernière entrée.
            const system_index = links.findIndex((link) => link.href === '/system.html');
            links.splice(system_index === -1 ? links.length : system_index, 0, entry);
            render();
        })
        .catch(() => {
            /* Service injoignable : on garde le menu de base, sans erreur visible. */
        });
});
