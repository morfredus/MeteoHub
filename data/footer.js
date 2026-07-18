document.addEventListener("DOMContentLoaded", () => {
    const footer = document.querySelector('footer');
    if (!footer) return;

    footer.innerHTML = `
        <span id="projectName">--</span> Web Interface - v<span id="version">--</span>
    `;

    fetch('/api/system')
        .then((response) => response.json())
        .then((data) => {
            const projectName = document.getElementById('projectName');
            const version = document.getElementById('version');

            if (projectName) {
                projectName.innerText = data.project_name || data.name || '--';
            }
            if (version) {
                version.innerText = data.project_version || data.version || '--';
            }
        })
        .catch((err) => console.error('Erreur infos système:', err));
});
