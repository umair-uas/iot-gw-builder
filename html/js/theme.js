document.addEventListener('DOMContentLoaded', function () {
    const themeToggle = document.getElementById('theme-toggle');
    if (!themeToggle) {
        console.error('Theme toggle button not found');
        return;
    }

    const currentTheme = localStorage.getItem('theme') || 'light';
    document.body.classList.add(`${currentTheme}-theme`);
    document.querySelector('.navbar').classList.add(`${currentTheme}-theme`);

    themeToggle.addEventListener('click', function () {
        const newTheme = document.body.classList.contains('light-theme') ? 'dark' : 'light';
        document.body.classList.toggle('light-theme');
        document.body.classList.toggle('dark-theme');
        document.querySelector('.navbar').classList.toggle('light-theme');
        document.querySelector('.navbar').classList.toggle('dark-theme');
        localStorage.setItem('theme', newTheme);
    });
});